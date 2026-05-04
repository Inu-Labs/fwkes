
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

#include <fwkes/rp2350/audio.h>
#include <fwkes/rp2350/joypads.h>
#include <fwkes/rp2350/leds.h>
#include <fwkes/rp2350/video.h>
#include <fwkes/rp2350/psram.h>
#include <fwkes/bus.h>
#include <fwkes/fs.h>

#include <platform.h>

#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <hardware/pio.h>
#include <hardware/sync.h>
#include <hardware/vreg.h>
#include <hardware/regs/qmi.h>
#include <hardware/regs/xip.h>
#include <hardware/structs/qmi.h>
#include <hardware/structs/xip_ctrl.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include <dvi.h>
#include <dvi_serialiser.h>

#include <stdio.h>
#include <string.h>

#ifdef PIMORONI_PICO_PLUS2_PSRAM_CS_PIN
#    define PSRAM_CS_PIN PIMORONI_PICO_PLUS2_PSRAM_CS_PIN
#endif

#define RESET_PIN 27
#define VREG_VSEL VREG_VOLTAGE_1_20
#define PICO_FREQ 300000 /* 300 MHz */

#define CYCLES_PER_FRAME (CPU_FREQ / 60)
#define CYCLES_PER_SCANLINE 114
#define DOTS_PER_SCANLINE 341
#define DOTS_UNTIL_NMI (241 * DOTS_PER_SCANLINE + 1)
#define FRAME_TIME (1000000000000 / 60)

#define LEFT_PADDING 32

typedef struct Emulator {
    Fs fs;
    Bus bus;
    bool reset_required;
} Emulator;

static Emulator g_emu;

/* clang-format off */
static uint16_t __attribute__((aligned(4))) g_colors_rgb565[64] = {
    /* 0       1       2       3       4       5       6       7       8       9       A       B       C       D       E       F */
    /* 0 */ 0x630c, 0x00f2, 0x1835, 0x4013, 0x600d, 0x6804, 0x6020, 0x48e0, 0x21a0, 0x0240, 0x0260, 0x0242, 0x01ab, 0x0000, 0x0000, 0x0000,
    /* 1 */ 0xad55, 0x0a7b, 0x397f, 0x70be, 0x9857, 0xb08c, 0xa920, 0x8a20, 0x5320, 0x23e0, 0x0440, 0x0406, 0x0372, 0x0000, 0x0000, 0x0000,
    /* 2 */ 0xffff, 0x553f, 0x843f, 0xb37f, 0xdf1f, 0xf318, 0xfbad, 0xdc84, 0xb560, 0x8640, 0x56a4, 0x3e8d, 0x3e19, 0x4a69, 0x0000, 0x0000,
    /* 3 */ 0xffff, 0xbf1f, 0xcebf, 0xe65f, 0xf63f, 0xfe1d, 0xfe59, 0xf695, 0xe6f3, 0xd753, 0xc775, 0xb778, 0xb75c, 0xbdf7, 0x0000, 0x0000
};
/* clang-format on */

static uint8_t g_framebuffer[PPU_HEIGHT][PPU_WIDTH];
static Ppu *g_ppu; /* for updating colors LUT in core 1 */

static void __not_in_flash_func(prepare_dvi_scanline)(void) {
    static uint16_t scanline[VIDEO_WIDTH] = {0};
    static unsigned curr_scanline;
    static uint16_t __attribute__((aligned(4))) colors_lut[32];

    if (g_ppu->colors_lut_dirty) {
        memcpy(colors_lut, g_ppu->colors_lut, 32 * sizeof(PpuPixel));
        g_ppu->colors_lut_dirty = false;
    }

    uint8_t *fb_scanline = g_framebuffer[curr_scanline];
    uint16_t *dvi_scanline_ptr = scanline + LEFT_PADDING;

    for (unsigned i = 0; i < PPU_WIDTH; ++i) {
        *dvi_scanline_ptr++ = colors_lut[*fb_scanline++ & 0x1f];
    }

    if (++curr_scanline == 240) {
        curr_scanline = 0;
    }

    const uint16_t *scanbuf = scanline;
    queue_add_blocking_u32(&g_dvi.q_colour_valid, &scanbuf);
    while (queue_try_remove_u32(&g_dvi.q_colour_free, &scanbuf))
        ;
}

static void __not_in_flash_func(prepare_ppu_scanline)(Ppu *ppu) {
    static unsigned curr_scanline = 0;

    memcpy(g_framebuffer[curr_scanline], ppu->scanline_buf, PPU_WIDTH);

    if (++curr_scanline == 240) {
        curr_scanline = 0;
    }
}

static void sample_callback(Apu *self, ApuSample smp) {
    (void) self;

    audio_queue(smp);
}

static void reset_callback(Bus *bus) {
    (void) bus;
    memset(g_framebuffer, 0, sizeof(g_framebuffer));
    g_emu.bus.cpu.halt_on_brk = false;
    g_emu.bus.ppu.scanline_cb = prepare_ppu_scanline;
    g_emu.bus.ppu.user_data = &g_emu;
}

static void reset_cb(uint gpio, uint32_t event_mask) {
    (void) event_mask;

    if (gpio != RESET_PIN) {
        return;
    }

    sleep_ms(25);

    if (!gpio_get(RESET_PIN)) {
        g_emu.reset_required = true;
    }
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
    }

    ppu->frame_done = false;

    /* The ultimate fix to prevent overflow of 32-bit cycle counter */
    if (cpu->cycles >= 1000000000) {
        cpu->cycles -= 1000000000;
        ppu->cycles -= 1000000000u * 3;
        apu->cycles -= 1000000000;
    }
}

static void blink_leds(void) {
    led_error_on();
    led_general_off();
    sleep_ms(500);

    led_error_off();
    led_general_on();
    sleep_ms(500);

    led_error_on();
    led_general_on();
    sleep_ms(500);

    led_error_off();
    led_general_off();
}

// #define FRAME_TIMES_NUM 60
// static unsigned g_frame_times[FRAME_TIMES_NUM];
// static unsigned g_frame_time_idx = 0;

void __attribute__((noreturn)) emu_loop(Emulator *self) {
    for (;;) {
        // unsigned start = time_us_32();

        update_joypads(&self->bus.joypad1, &self->bus.joypad2);

        disable_interrupts();

        if (self->reset_required) {
            BusEvent new_ev = {
                .id = BUS_EVENT_RESET,
            };

            bus_add_event(&self->bus, &new_ev);

            self->reset_required = false;
        }

        enable_interrupts();

        bus_update(&self->bus);

        run_frame(self);

        // unsigned end = time_us_32();
        //
        // g_frame_times[g_frame_time_idx++] = end - start;
        //
        // if (g_frame_time_idx >= FRAME_TIMES_NUM) {
        //     g_frame_time_idx = 0;
        //
        //     unsigned avg_ft = 0;
        //
        //     for (unsigned i = 0; i < FRAME_TIMES_NUM; ++i) {
        //         avg_ft += g_frame_times[i];
        //     }
        //
        //     avg_ft = avg_ft / FRAME_TIMES_NUM;
        //
        //     printf("ft = %u\n", avg_ft);
        // }
    }
}

int main(void) {
    vreg_set_voltage(VREG_VSEL);
    sleep_ms(10);
    set_sys_clock_khz(PICO_FREQ, true);

    stdio_init_all();
    printf("\n");
    sleep_ms(1000);

    printf("FWKES - a NES emulator\n");
    printf("Emu Firmware v0.1.0\n");

    printf("Initializing SPI...\n");
    spi_setup();

    gpio_init(LED_GENERAL_PIN);
    gpio_init(LED_ERROR_PIN);

    gpio_set_dir(LED_GENERAL_PIN, GPIO_OUT);
    gpio_set_dir(LED_ERROR_PIN, GPIO_OUT);

    printf("Blinking LEDs just for sake of fun...\n");

    blink_leds();

    gpio_init(RESET_PIN);
    gpio_set_dir(RESET_PIN, GPIO_IN);
    gpio_set_irq_enabled_with_callback(
        RESET_PIN, GPIO_IRQ_EDGE_FALL, true, reset_cb
    );

    init_joypads();

#ifdef PSRAM_CS_PIN
    if (!psram_init(PSRAM_CS_PIN)) {
        printf("cannot initialize PSRAM\n");
    }
#endif

    printf("Initializing filesystem...\n");

    FsError res = fs_init(&g_emu.fs);

    if (res != FS_ERR_OK) {
        printf("Cannot initialize filesystem\n");
        goto failure;
    } else {
        printf("Successfully initialized fs\n");
    }

    if (!bus_init(&g_emu.bus, &g_emu.fs)) {
        fs_free(&g_emu.fs);
        printf("Cannot initialize emulator\n");
        goto failure;
    }

    g_ppu = &g_emu.bus.ppu;

    g_emu.bus.reset_cb = reset_callback;
    g_emu.bus.user_data = &g_emu;
    g_emu.bus.ppu.scanline_cb = prepare_ppu_scanline;
    g_emu.bus.ppu.user_data = NULL;
    g_emu.bus.apu.sample_cb = sample_callback;
    g_emu.bus.apu.user_data = &g_emu;
    g_emu.bus.ppu.colors = g_colors_rgb565;

    ppu_init_pixel_luts(&g_emu.bus.ppu);

    printf("Initializing APU DMA channel...\n");

    audio_init();

    printf("Loading BIOS...\n");
    if (!bus_load_disk(&g_emu.bus, "0:bios.nes")) {
        printf("Cannot load BIOS\n");
        fs_free(&g_emu.fs);

        goto failure;
    }

    printf("BIOS successfully loaded\n");

    video_init(prepare_dvi_scanline);
    prepare_dvi_scanline();

    emu_loop(&g_emu);

failure:
    fs_free(&g_emu.fs);
    led_error_on();
    for (;;);
}
