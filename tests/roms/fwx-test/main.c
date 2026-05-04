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

#include "fwx_public.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// the NES's Picture Processing Unit (PPU) exposes memory-mapped registers to
// the CPU at these locations:
#define PPU_CTRL                                                               \
    *(                                                                         \
        (unsigned char *) 0x2000                                               \
    ) // control register: set bit flags to control how the PPU behaves
#define PPU_MASK                                                               \
    *((unsigned char *) 0x2001) // mask register: set bit flags to control
                                // sprite rendering/color effects
#define PPU_STATUS                                                             \
    *((unsigned char *) 0x2002) // status register: reflects the state of
                                // various functions inside the PPU
#define PPU_SCROLL                                                             \
    *((unsigned char *) 0x2005) // scroll register: indicates which pixel of the
                                // nametable should be at (0,0) on the screen
#define PPU_ADDRESS                                                            \
    *(                                                                         \
        (unsigned char *) 0x2006                                               \
    ) // address register: points to the address in video memory where we want
      // PPU_DATA writes to go
#define PPU_DATA                                                               \
    *((unsigned char *) 0x2007) // data register: writes here store that value
                                // at *(PPU_ADDRESS) in video memory

// color palette: https://wiki.nesdev.com/w/index.php/PPU_palettes#2C02
#define COLOR_BLACK 0x1f
#define COLOR_GRAY 0x00
#define COLOR_LIGHTGRAY 0x10
#define COLOR_WHITE 0x20

static const unsigned char TEXT[] = {"NIGGA HEIL HITLER!~"};
static const unsigned char PALETTE[] = {
    COLOR_BLACK, COLOR_WHITE, COLOR_WHITE, COLOR_WHITE
};

static void vram_write_xy(unsigned x, unsigned y, uint8_t data) {
    uint16_t addr = 0x2000 | (x & 0x1f) | ((y & 0x1f) << 5);
    PPU_ADDRESS = (addr & 0xff00) >> 8;
    PPU_ADDRESS = addr & 0xff;
    PPU_DATA = data;
}

static void vram_write_cstr(unsigned x, unsigned y, const char *path) {
    unsigned i, n;

    n = (unsigned) strlen(path);

    for (i = 0; i < n; ++i) {
        unsigned x1 = x + i;

        if (x1 >= 32) {
            break;
        }

        vram_write_xy(x1, y, (uint8_t) path[i]);
    }
}

int main(void) {
    static int file_count;
    static char filename[256];
    static char idx_str[3];
    int i, j;
    unsigned filename_len = 0;
    bool is_dir;

    // turn off the screen
    PPU_CTRL = 0x00;
    PPU_MASK = 0x00;

    // load the palette at PPU memory address 0x3f00, which is where it stores
    // backgrounds https://wiki.nesdev.com/w/index.php/PPU_palettes#Memory_Map
    PPU_ADDRESS = 0x3f;
    PPU_ADDRESS = 0x00;

    for (i = 0; i < sizeof(PALETTE); ++i) {
        PPU_DATA = PALETTE[i];
    }

    // load the text at address 0x21ca,
    // placing it about in the center of the screen
    // PPU_ADDRESS = 0x21;
    // PPU_ADDRESS = 0xca;
    // for (index = 0; index < sizeof(TEXT); ++index) {
    //     PPU_DATA = (unsigned char) filename[index];
    // }

    for (i = 0; i < 30; ++i) {
        for (j = 0; j < 32; ++j) {
            vram_write_xy(j, i, (uint8_t) ' ');
        }
    }

    fwx_start();

    fwx_write_u8(FWX_CMD_SD_FILE_COUNT);
    file_count = fwx_read_u8();

    for (i = 0; i < file_count && i < 30; ++i) {
        fwx_write_u8(FWX_CMD_SD_IS_DIR_IDX);
        fwx_write_u8(i & 0xff);
        is_dir = fwx_read_u8();

        fwx_write_u8(FWX_CMD_SD_FILENAME_IDX);
        fwx_write_u8(i & 0xff);

        itoa(i + 1, idx_str, 10);
        vram_write_cstr(0, i, idx_str);
        vram_write_cstr(2, i, " ");

        for (j = 0; FWXSTAT & FWX_STAT_S1; ++j) {
            filename[j] = (char) fwx_read_u8();
            // uint8_t ch = fwx_read_u8();
            //
            // if (is_dir && (ch == '\0' || j == 29)) {
            //     vram_write_xy(3 + j, i, '/');
            // } else {
            //     vram_write_xy(3 + j, i, ch);
            // }
        }

        filename_len = strlen(filename);

        for (j = 0; j < filename_len + 1; ++j) {
            char ch = filename[j];

            if (ch == '\0' || j == 29) {
                if (is_dir) {
                    vram_write_xy(3 + j, i, '/');
                }
                break;
            } else {
                vram_write_xy(3 + j, i, ch);
            }
        }
    }

    fwx_write_u8(FWX_CMD_SD_LOAD_IDX);
    fwx_write_u8(17);

    fwx_stop();

    // reset scroll position
    PPU_SCROLL = 0x00; // horizontal offset
    PPU_SCROLL = 0x00; // vertical offset

    // turn on the screen
    PPU_CTRL = 0x80;
    PPU_MASK = 0x1e; // show sprites and background in color

    // display the text forever
    for (;;) {
    }

    return 0;
}
