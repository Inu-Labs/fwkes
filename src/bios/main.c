/*
 * Copyright (C) 2025-present InuLabs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "fwkes/fwx/common.h"
#include "fwkes/fwx/public.h"

#include "nesdoug.h"
#include "neslib.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define NAMETABLE NAMETABLE_A
#define ATTABLE (NAMETABLE | 0x03c0)

#define FILES_PER_PAGE 26
#define PAGE_Y 3

static char filename[30];
static uint8_t filename_len;
static int cursor;
static int old_cursor;
static uint8_t page;
static uint8_t old_page;
static uint8_t file_count;
static unsigned i, j;
static bool is_dir;
static uint8_t pad1;
static uint8_t pad1_last;
static uint8_t pad1_pressed;
static uint8_t y;
uint8_t type;

static const unsigned char palette_bg[] = {
    0x0f, 0x30, 0x30, 0x30, 0x0f, 0x30, 0x30, 0x28,
    0x0f, 0x30, 0x30, 0x30, 0x0f, 0x30, 0x30, 0x30,
};

static const unsigned char palette_credits[] = {
    0x0f, 0x11, 0x28, 0x21,
    0x0f, 0x30, 0x30, 0x30,
    0x0f, 0x30, 0x30, 0x30,
    0x0f, 0x30, 0x30, 0x30,
};

static void print_at(unsigned x, unsigned y, const char *text) {
    vram_adr(NTADR_A(x, y));

    while (*text) {
        vram_put((unsigned char) *text);
        ++text;
    }
}

static void draw_header(void) {
    print_at(0, 0, "BIOS v1.0         (C) A. Kachaev");
    print_at(0, 1, "Emu FW v1.0           J. Jansa");
}

static void fetch_filename(uint8_t idx) {
    fwx_write_u8(FWX_CMD_SD_GET_ENTRY);
    fwx_write_u8(idx);

    type = fwx_read_u8();
    is_dir = (type > 0);

    filename_len = 0;

    while (FWXSTAT & FWX_STAT_S1) {
        char c = (char)fwx_read_u8();

        if (c == 0) break;

        if (filename_len < 29) {
            filename[filename_len++] = c;
        }
    }

    if (is_dir && filename_len < 29) {
        filename[filename_len++] = '/';
    }

    filename[filename_len] = '\0';

}

static void clear_screen(void) {
    vram_adr(NAMETABLE);
    vram_fill(' ', 960);

    vram_adr(ATTABLE);
    vram_fill(0x00, 64);
}

static void clear_page_area(void) {
    vram_adr(NAMETABLE);

    for (i = PAGE_Y; i < 30; ++i) {
        vram_adr(NTADR_A(0, i));

        vram_fill(' ', 32);
    }
}

static void draw_filename(uint8_t idx, bool loaded) {
    static char idx_str[4];

    y = PAGE_Y + idx % FILES_PER_PAGE;
    itoa(idx + 1, idx_str, 10);
    idx_str[3] = 0;
    print_at(0, y, idx_str);

    vram_adr(NTADR_A(4, y));

    for (j = 0; filename[j]; ++j) {
        uint8_t ch = (uint8_t) filename[j];
        vram_put(loaded ? ch + 0x70 : ch);
    }
}

static void draw_page(void) {
    unsigned idx = 0;

    for (i = 0; i < FILES_PER_PAGE; ++i) {
        idx = page * FILES_PER_PAGE + i;

        if (idx >= file_count) {
            break;
        }

        fetch_filename(idx);
        draw_filename(idx, false);
    }
}

static void draw_cursor(void) {
    oam_clear();
    y = PAGE_Y + cursor % FILES_PER_PAGE;
    oam_spr(24, y * 8, 0x80, 0x01);

    // y = PAGE_Y + old_cursor % FILES_PER_PAGE;
    //
    // vram_adr(NTADR_A(3, y));
    // vram_put(' ');
    //
    // y = PAGE_Y + cursor % FILES_PER_PAGE;
    //
    // vram_adr(NTADR_A(3, y));
    // // vram_put('}');
    // vram_put('\x80');
}

static void init(void) {
    ppu_off();
    pal_bg(palette_bg);
    pal_spr(palette_bg);
    bank_spr(0);

    clear_screen();
    draw_header();

    fwx_start();
    fwx_write_u8(FWX_CMD_SD_FILE_COUNT);
    file_count = fwx_read_u8();

    draw_page();
    draw_cursor();

    ppu_on_all();
}

static void read_input(void) {
    pad1_last = pad1;
    pad1 = pad_poll(0);
    pad1_pressed = pad1 & ~pad1_last;
}

static void wrap_cursor(void) {
    if (cursor < 0) {
        cursor = file_count - 1;
    } else if (cursor >= file_count) {
        cursor = 0;
    }
}

static uint8_t cursor_within_page(void) {
    return cursor / FILES_PER_PAGE;
}

static void move_cursor(int delta) {
    old_cursor = cursor;
    old_page = page;

    cursor += delta;
    wrap_cursor();

    page = cursor_within_page();

    if (page != old_page) {
        ppu_off();
        clear_page_area();
        draw_page();
        ppu_on_all();
    }

    draw_cursor();
}

static void draw_loading(void) {
    clear_screen();
    print_at(11, 14, "LOADING");
}
static void load_rom() {

    ppu_off();
    oam_clear();
    draw_loading();
    ppu_on_all();
    delay(5);

    fwx_write_u8(FWX_CMD_SD_LOAD);
    fwx_write_u8(cursor & 0xff);

    if (fwx_last_error() != FWX_ERR_OK) {
        fwx_write_u8(FWX_CMD_LED_ERROR);
        fwx_write_u8(1);

        ppu_off();
        clear_screen();
        draw_header();
        draw_page();
        draw_cursor();
        ppu_on_all();

        return;
    }

    ppu_off();

    fwx_write_u8(FWX_CMD_SD_FILE_COUNT);
    file_count = fwx_read_u8();

    old_cursor = 0;
    cursor = 0;
    page = 0;
    old_page = 0;

    clear_screen();
    draw_header();
    draw_page();
    draw_cursor();

    ppu_on_all();
}

void show_credits() {
    ppu_off();
    oam_clear();
    clear_screen();

    pal_bg(palette_credits);

    vram_adr(NTADR_A(1, 4));  vram_fill('-', 30);
    vram_adr(NTADR_A(1, 24)); vram_fill('-', 30);

    for(i = 5; i < 24; ++i) {
        vram_adr(NTADR_A(1, i));  vram_put('|');
        vram_adr(NTADR_A(30, i)); vram_put('|');
    }
    vram_adr(NTADR_A(1, 4));   vram_put('+');
    vram_adr(NTADR_A(30, 4));  vram_put('+');
    vram_adr(NTADR_A(1, 24));  vram_put('+');
    vram_adr(NTADR_A(30, 24)); vram_put('+');

    print_at(7, 8, "On project worked:");

    print_at(11, 11, "A. Kachaev");
    print_at(12, 12, "J. Jansa");

    print_at(12, 17, "SOC 2026");

    print_at(4, 21, "github.com/inunix3/fwkes");
    ppu_wait_nmi();
    ppu_on_all();

    while(pad_poll(0) != 0);

    while(pad_poll(0) == 0);

    ppu_off();
    pal_bg(palette_bg);
    clear_screen();
    draw_header();
    draw_page();
    draw_cursor();
    ppu_on_all();
}

static void handle_input(void) {
    if (pad1_pressed & PAD_DOWN) {
        move_cursor(+1);
    } else if (pad1_pressed & PAD_UP) {
        move_cursor(-1);
    } else if (pad1_pressed & PAD_LEFT) {
        move_cursor(-FILES_PER_PAGE);
    } else if (pad1_pressed & PAD_RIGHT) {
        move_cursor(+FILES_PER_PAGE);
    } else if (pad1_pressed & PAD_START) {
        load_rom();
    } else if(pad1_pressed & PAD_A){
        show_credits();
    }
}

void main(void) {
    init();

    for (;;) {
        ppu_wait_nmi();

        read_input();
        handle_input();
    }
}
