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

#include <fwkes/bus.h>
#include <fwkes/disk.h>
#include <fwkes/cpu.h>

#include <SDL3/SDL.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static uint32_t g_colors[0x100];

typedef struct Emulator {
    Bus bus;
    Cpu cpu;
    uint8_t memory[MEMORY_SIZE];
    Disk disk;
    Fs fs;

    SDL_Window *win;
    SDL_Renderer *renderer;
    SDL_Texture *canvas;

    uint8_t pixels[32][32];
} Emulator;

static void prepare_canvas(Emulator *self) {
    SDL_SetRenderTarget(self->renderer, self->canvas);
    SDL_SetRenderScale(self->renderer, 10.f, 10.f);

    for (int y = 0; y < 32; ++y) {
        for (int x = 0; x < 32; ++x) {
            SDL_FRect rect = {(float) x, (float) y, 1, 1};

            uint32_t color = g_colors[self->pixels[y][x]];

            SDL_SetRenderDrawColor(
                self->renderer, (color & 0xff0000) >> 16,
                (color & 0x00ff00) >> 8, color & 0x0000ff, 0xff
            );

            SDL_RenderFillRect(self->renderer, &rect);
        }
    }

    SDL_SetRenderTarget(self->renderer, NULL);
}

bool write_handler(Bus *self, uint16_t address, uint8_t data) {
    Emulator *emu = self->user_data;

    if (address >= 0x0200 && address < 0x0600) {
        emu->pixels[(address - 0x0200) / 32][(address - 0x0200) % 32] = data;

        prepare_canvas(emu);
    } else {
        return false;
    }

    return true;
}

bool read_handler(const Bus *self, uint16_t address, uint8_t *data) {
    const Emulator *emu = self->user_data;

    if (address >= 0x0200 && address < 0x0600) {
        *data = emu->pixels[(address - 0x0200) / 32][(address - 0x0200) % 32];
    } else if (address == 0xfe) {
        *data = rand() & 0xff;
    } else {
        return false;
    }

    return true;
}

void emu_init(Emulator *self) {
    for (int i = 0; i < 0xff; ++i) {
        g_colors[i] = 0x00ffff;
    }

    g_colors[0] = 0x000000;
    g_colors[1] = 0xffffff;
    g_colors[2] = 0x595959;
    g_colors[3] = 0xb3b3b3;
    g_colors[4] = 0x7f0000;
    g_colors[5] = 0xff0000;
    g_colors[6] = 0x007f00;
    g_colors[7] = 0x00ff00;
    g_colors[8] = 0x00007f;
    g_colors[9] = 0x0000ff;
    g_colors[10] = 0x7f007f;
    g_colors[11] = 0xff00ff;
    g_colors[12] = 0x7f7f00;
    g_colors[13] = 0xffff00;
    g_colors[14] = 0x007f7f;

    memset(self, 0, sizeof(*self));

    fs_init(&self->fs);

    if (!disk_load(&self->disk, &self->fs, "6502_snake.nes")) {
        printf("cannot load 6502_snake.nes\n");

        exit(-1);
    }

    self->bus.memory = self->memory;
    self->bus.disk = &self->disk;
    self->bus.write_handler = write_handler;
    self->bus.read_handler = read_handler;
    self->bus.user_data = self;

    self->cpu.bus = &self->bus;
    cpu_init(&self->cpu, &self->bus);
    self->cpu.halt_on_brk = true;
}

void emu_render(Emulator *self) {
    SDL_RenderTexture(self->renderer, self->canvas, NULL, NULL);
}

void emu_run(Emulator *self) {
    while (!self->cpu.halt) {
        SDL_Event ev;

        while (SDL_PollEvent(&ev)) {

            switch (ev.type) {
            case SDL_EVENT_QUIT:
                return;
            case SDL_EVENT_KEY_DOWN:
                switch (ev.key.scancode) {
                case SDL_SCANCODE_W:
                    self->memory[0xff] = 0x77;

                    break;
                case SDL_SCANCODE_S:
                    self->memory[0xff] = 0x73;

                    break;
                case SDL_SCANCODE_A:
                    self->memory[0xff] = 0x61;

                    break;
                case SDL_SCANCODE_D:
                    self->memory[0xff] = 0x64;

                    break;
                default:
                    break;
                }
            default:
                break;
            }
        }

        cpu_step(&self->cpu);

        SDL_SetRenderDrawColor(self->renderer, 0x00, 0x00, 0x00, 0xff);
        SDL_RenderClear(self->renderer);

        emu_render(self);

        SDL_RenderPresent(self->renderer);

        SDL_DelayPrecise(70000);
    }
}

int main(void) {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        return -1;
    }

    Emulator emu;
    emu_init(&emu);

    srand((unsigned int) time(NULL));

    if (!SDL_CreateWindowAndRenderer(
            "6502 Snake", 320, 320, 0, &emu.win, &emu.renderer
        )) {
        SDL_Quit();

        return -1;
    }

    emu.canvas = SDL_CreateTexture(
        emu.renderer, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_TARGET, 320, 320
    );

    emu_run(&emu);

    disk_unload(&emu.disk);
    fs_free(&emu.fs);

    SDL_DestroyTexture(emu.canvas);
    SDL_DestroyRenderer(emu.renderer);
    SDL_DestroyWindow(emu.win);

    SDL_Quit();

    return 0;
}
