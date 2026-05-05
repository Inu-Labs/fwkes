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

#include <fwkes/apu.h>
#include <fwkes/bus.h>
#include <fwkes/cpu.h>
#include <fwkes/disk.h>
#include <fwkes/fwx/private.h>
#include <fwkes/log.h>
#include <fwkes/ppu.h>
#include <fwkes/trace.h>

#include <SDL3/SDL.h>

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SAMPLE_RATE 44100
#define AUDIO_BUF_SIZE 128 /* has to be power of 2! */
#define FRAME_TIME (1000000000 / 60)
#define CYCLES_PER_FRAME (CPU_FREQ / 60)
#define CYCLES_PER_SCANLINE 113
#define CYCLES_PER_SAMPLE (CPU_FREQ / SAMPLE_RATE)
#define CYCLES_PER_SAMPLE_F64 ((double) CPU_FREQ / (double) SAMPLE_RATE)
#define DOTS_PER_SCANLINE 341
#define DOTS_UNTIL_NMI (241 * DOTS_PER_SCANLINE + 1)

/* clang-format off */
static PpuPixel g_colors[64] = {
    /*          0         1        2          3         4         5         6         7         8         9         A         B         C         D         E         F */
    /* 0 */ 0x626262ff, 0x001c95ff, 0x1904acff, 0x42009dff, 0x61006bff, 0x6e0025ff, 0x650500ff, 0x491e00ff, 0x223700ff, 0x004900ff, 0x004f00ff, 0x004816ff, 0x00355eff, 0x000000ff, 0x000000ff, 0x000000ff,
    /* 1 */ 0xabababff, 0x0c4edbff, 0x3d2effff, 0x7115f3ff, 0x9b0bb9ff, 0xb01262ff, 0xa92704ff, 0x894600ff, 0x576600ff, 0x237f00ff, 0x008900ff, 0x008332ff, 0x006d90ff, 0x000000ff, 0x000000ff, 0x000000ff,
    /* 2 */ 0xffffffff, 0x57a5ffff, 0x8287ffff, 0xb46dffff, 0xdf60ffff, 0xf863c6ff, 0xf8746dff, 0xde9020ff, 0xb3ae00ff, 0x81c800ff, 0x56d522ff, 0x3dd36fff, 0x3ec1c8ff, 0x4e4e4eff, 0x000000ff, 0x000000ff,
    /* 3 */ 0xffffffff, 0xbee0ffff, 0xcdd4ffff, 0xe0caffff, 0xf1c4ffff, 0xfcc4efff, 0xfdcaceff, 0xf5d4afff, 0xe6df9cff, 0xd3e99aff, 0xc2efa8ff, 0xb7efc4ff, 0xb6eae5ff, 0xb8b8b8ff, 0x000000ff, 0x000000ff
};
/* clang-format on */

typedef struct KeyMap {
    SDL_Scancode scancode;
    Button button;
} KeyMap;

/* Keymaps like in Mesen2 */
static const KeyMap keymap[] = {
    {SDL_SCANCODE_K, BTN_A},      {SDL_SCANCODE_J, BTN_B},
    {SDL_SCANCODE_U, BTN_SELECT}, {SDL_SCANCODE_I, BTN_START},
    {SDL_SCANCODE_W, BTN_UP},     {SDL_SCANCODE_S, BTN_DOWN},
    {SDL_SCANCODE_A, BTN_LEFT},   {SDL_SCANCODE_D, BTN_RIGHT}
};

static uint8_t joypad1_state;
static uint8_t joypad1_index;

uint8_t read_joypad1(void) {
    uint8_t ret = (joypad1_state >> joypad1_index) & 1;
    joypad1_index = (joypad1_index + 1) & 7;
    return ret;
}

static void update_joypad_state(Joypad *joypad, const Uint8 *keyboard_state) {
    joypad->state = 0;
    for (size_t i = 0; i < sizeof(keymap) / sizeof(keymap[0]); ++i) {
        if (keyboard_state[keymap[i].scancode]) {
            joypad->state |= keymap[i].button;
        }
    }
}

typedef struct Emulator {
    Bus bus;
    Fs fs;

    SDL_Window *win;
    SDL_Renderer *renderer;
    SDL_AudioStream *stream;
    ApuSample audio_buf[AUDIO_BUF_SIZE];
    size_t sample_count;
    double raw_sample;
    double raw_sample_count;
    SDL_Texture *canvas;
    PpuPixel scanline_buf[PPU_WIDTH];
    unsigned scanline;
    uint32_t framebuffer[PPU_HEIGHT][PPU_WIDTH];
    unsigned px_x;
    unsigned px_y;
    bool quit;
    bool draw_grid;
    bool frame_done;
} Emulator;

static void scanline_callback(Ppu *ppu) {
    static uint32_t colors_lut[32];
    Emulator *self = ppu->user_data;

    if (ppu->colors_lut_dirty) {
        memcpy(colors_lut, ppu->colors_lut, 32 * sizeof(PpuPixel));
        ppu->colors_lut_dirty = false;
    }

    for (int i = 0; i < PPU_WIDTH; ++i) {
        uint8_t px = ppu->scanline_buf[i];
        self->framebuffer[self->scanline][i] = colors_lut[px & 0x1f];
    }

    if (++self->scanline == 240) {
        self->frame_done = true;
        self->scanline = 0;
    }
}

void emu_reset(Emulator *self);

static void reset_callback(Bus *bus) {
    Emulator *emu = bus->user_data;
    emu_reset(emu);
}

static void sample_callback(Apu *self, ApuSample smp) {
    Emulator *emu = self->user_data;

    emu->audio_buf[emu->sample_count] = smp;

    if (++emu->sample_count >= AUDIO_BUF_SIZE) {
        SDL_PutAudioStreamData(
            emu->stream, emu->audio_buf, AUDIO_BUF_SIZE * sizeof(ApuSample)
        );

        emu->sample_count = 0;
    }
}

void emu_init(Emulator *self) {
    // log_msg(LOG_INFO, "initializing emulator...");

    memset(self, 0, sizeof(*self));

    trace_add_cat_filter(TRACE_PPU);
    trace_add_cat_filter(TRACE_FWX);
    log_add_cat_filter(LOG_PPU);
    log_add_cat_filter(LOG_BUS);
    // trace_add_type_filter(
    //     TRACE_MSG | TRACE_ERROR | TRACE_WARN | TRACE_REG_READ |
    //     TRACE_REG_WRITE | TRACE_W_LATCH | TRACE_VBLANK | TRACE_NMI
    // );

    fs_init(&self->fs);

    char bios_path[256];
    fs_pwd(&self->fs, bios_path, 255);
    strncat(bios_path, "/bios.nes", 256);

    bus_init(&self->bus, &self->fs, bios_path);

    self->bus.reset_cb = reset_callback;
    self->draw_grid = false;
    // self->bus.cpu.disas = false;
    self->bus.cpu.halt_on_brk = false;
    self->bus.ppu.colors = g_colors;
    self->bus.ppu.scanline_cb = scanline_callback;
    self->bus.ppu.user_data = self;
    self->bus.user_data = self;
    self->bus.apu.sample_cb = sample_callback;
    self->bus.apu.user_data = self;
    ppu_init_pixel_luts(&self->bus.ppu);

    if (!bus_load_disk(&self->bus, bios_path)) {
        log_msg(LOG_ERROR, "cannot load BIOS file");

        exit(-1);
    }
}

void emu_reset(Emulator *self) {
    memset(self->framebuffer, 0, sizeof(self->framebuffer));
    self->frame_done = false;
    self->scanline = 0;
    self->sample_count = 0;
    SDL_ClearAudioStream(self->stream);

    while (SDL_GetAudioStreamQueued(self->stream) > 0)
        ;

    self->bus.cpu.halt_on_brk = false;
    self->bus.ppu.colors = g_colors;
    self->bus.ppu.scanline_cb = scanline_callback;
    self->bus.ppu.user_data = self;
}

void emu_render(Emulator *self) {
    SDL_UpdateTexture(
        self->canvas, NULL, self->framebuffer, 256 * sizeof(uint32_t)
    );

    SDL_RenderTexture(self->renderer, self->canvas, NULL, NULL);

    if (self->draw_grid) {
        for (int i = 0; i < PPU_HEIGHT / 8; ++i) {
            for (int j = 0; j < PPU_WIDTH / 8; ++j) {
                SDL_FRect rect = {
                    (float) j * 16.f, (float) i * 16.f, 16.f, 16.f
                };

                SDL_SetRenderDrawColor(self->renderer, 0xff, 0xff, 0xff, 0xff);
                SDL_RenderRect(self->renderer, &rect);
            }
        }
    }

    self->frame_done = false;
}

void emu_handle_events(Emulator *self) {
    SDL_Event ev;
    while (SDL_PollEvent(&ev)) {
        if (ev.type == SDL_EVENT_QUIT) {
            self->quit = true;
        } else if (ev.type == SDL_EVENT_KEY_DOWN &&
                   ev.key.scancode == SDL_SCANCODE_ESCAPE) {
            self->quit = true;
        } else if (ev.type == SDL_EVENT_KEY_DOWN &&
                   ev.key.scancode == SDL_SCANCODE_G) {
            self->draw_grid = !self->draw_grid;
        } else if (ev.type == SDL_EVENT_KEY_DOWN &&
                   ev.key.scancode == SDL_SCANCODE_R) {
            BusEvent new_ev = {
                .id = BUS_EVENT_RESET,
            };

            bus_add_event(&self->bus, &new_ev);
        }
    }

    const Uint8 *keyboard_state = (const Uint8 *) SDL_GetKeyboardState(NULL);
    update_joypad_state(&self->bus.joypad1, keyboard_state);
    update_joypad_state(&self->bus.joypad2, keyboard_state);
}

static void run_frame(Emulator *self) {
    Cpu *cpu = &self->bus.cpu;
    Ppu *ppu = &self->bus.ppu;
    Apu *apu = &self->bus.apu;

    while (!ppu->frame_done) {
        /* CPU will run until some significant event (vblank/nmi or end of
         * frame)
         */

        CycleCounter curr_ppu_abs_dot =
            ppu->scanline * DOTS_PER_SCANLINE + ppu->dot;
        CycleCounter target_ppu_abs_dot;

        if (self->bus.disk.mapper_hsync && ppu->scanline < 240) {
            if (ppu->dot < 260) {
                /* Target the current scanline's IRQ dot */
                target_ppu_abs_dot = ppu->scanline * DOTS_PER_SCANLINE + 260;
            } else {
                /* We passed it, target the NEXT scanline's IRQ dot */
                target_ppu_abs_dot =
                    (ppu->scanline + 1) * DOTS_PER_SCANLINE + 260;
            }
        } else if (ppu->scanline < 241) {
            /* next event is vblank */
            target_ppu_abs_dot = 241 * DOTS_PER_SCANLINE + 1;
        } else {
            /* next event is end of frame */
            target_ppu_abs_dot = 262 * DOTS_PER_SCANLINE;
        }

        CycleCounter cycles_to_event =
            (target_ppu_abs_dot - curr_ppu_abs_dot) / 3;
        CycleCounter event_deadline = cpu->cycles + cycles_to_event;
        CycleCounter cycles_to_run = cpu->cycles + CYCLES_PER_SCANLINE;

        /* significant event has higher priority */
        if (event_deadline < cycles_to_run) {
            cycles_to_run = event_deadline;
        }

        /* prevent infinite loop */
        if (cycles_to_run <= cpu->cycles) {
            cycles_to_run = cpu->cycles + 1;
        }

        cpu_run_until(cpu, cycles_to_run);
        ppu_run_until(ppu, cpu->cycles * 3);
        apu_run_until(apu, cpu->cycles);

        // bus_hsync(&self->bus);
    }

    ppu->frame_done = false;
}

void emu_run(Emulator *self) {
    while (!self->quit) {
        uint64_t start_time = SDL_GetTicksNS();

        emu_handle_events(self);

        bus_update(&self->bus);
        run_frame(self);

        SDL_SetRenderDrawColor(self->renderer, 0x00, 0x00, 0x00, 0xff);
        SDL_RenderClear(self->renderer);

        if (self->frame_done) {
            emu_render(self);
        }

        SDL_RenderPresent(self->renderer);

        uint64_t end_time = start_time + FRAME_TIME;
        uint64_t now = SDL_GetTicksNS();

        if (now < end_time) {
            SDL_DelayPrecise(end_time - now);
        }
    }
}

int main(void) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO)) {
        return -1;
    }

    Emulator emu;
    emu_init(&emu);

    if (!SDL_CreateWindowAndRenderer(
            "FWKES (Desktop)", 256 * 2, 240 * 2, 0, &emu.win, &emu.renderer
        )) {
        SDL_Quit();

        return -1;
    }

    emu.canvas = SDL_CreateTexture(
        emu.renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STATIC, 256,
        240
    );

    SDL_SetTextureScaleMode(emu.canvas, SDL_SCALEMODE_NEAREST);

    const SDL_AudioSpec spec = {SDL_AUDIO_S16, 1, SAMPLE_RATE};
    emu.stream = SDL_OpenAudioDeviceStream(
        SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL
    );

    if (!emu.stream) {
        SDL_Log("Couldn't open audio device stream: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_ResumeAudioStreamDevice(emu.stream);

    emu_run(&emu);

    // log_msg(LOG_INFO, "freeing all resources");

    SDL_DestroyTexture(emu.canvas);
    SDL_DestroyRenderer(emu.renderer);
    SDL_DestroyWindow(emu.win);
    SDL_DestroyAudioStream(emu.stream);
    bus_deinit(&emu.bus);

    SDL_Quit();

    return 0;
}
