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

#include <fwkes/rp2350/video.h>

#include <dvi_serialiser.h>

#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/irq.h>
#include <hardware/pio.h>
#include <hardware/sync.h>
#include <pico/multicore.h>
#include <pico/stdlib.h>

#include <stdbool.h>
#include <stdio.h>

#define DVI_TIMING dvi_timing_640x480p_60hz
#define DEFAULT_SERIAL_CONFIG pico_sock_cfg
#define DEFAULT_PIO_INST pio0

struct dvi_inst g_dvi;

static const struct dvi_serialiser_cfg pico_sock_cfg = {
    .pio = DEFAULT_PIO_INST,
    .sm_tmds = {0, 1, 2},
    .pins_tmds = {12, 18, 16},
    .pins_clk = 14,
    .invert_diffpairs = false
};

static void core1_main() {
    dvi_register_irqs_this_core(&g_dvi, DMA_IRQ_0);
    dvi_start(&g_dvi);
    dvi_scanbuf_main_16bpp(&g_dvi);
    __builtin_unreachable();
}

void video_init(dvi_callback_t scanline_cb) {
    printf("Initializing DVI...\n");

    g_dvi.timing = &DVI_TIMING;
    g_dvi.ser_cfg = DEFAULT_SERIAL_CONFIG;
    g_dvi.scanline_callback = scanline_cb;

    dvi_init(
        &g_dvi, next_striped_spin_lock_num(), next_striped_spin_lock_num()
    );

    multicore_launch_core1(core1_main);

    printf("DVI successfully initialized\n");
}
