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

#include <fwkes/rp2350/joypads.h>

#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <pico/stdlib.h>

#include <joypad_read.pio.h>

#include <stdio.h>

static PIO g_pio;
static unsigned g_sm; /* sm for state machine */
static unsigned g_offset;

void init_joypads(void) {
    printf("Initializing joypads...\n");

    pio_claim_free_sm_and_add_program(
        &joypad_read_program, &g_pio, &g_sm, &g_offset
    );

    joypad_read_program_init(
        g_pio, g_sm, g_offset, JOYPAD_STR_PIN, JOYPAD_CLK_PIN, JOYPAD1_OUT_PIN
    );
}

void update_joypads(Joypad *joy1, Joypad *joy2) {
    /* SYNC: in case PIO goes out of sync, we clear out the FIFO queue. */
    while (!pio_sm_is_rx_fifo_empty(g_pio, g_sm)) {
        pio_sm_get(g_pio, g_sm);
    }

    if (!pio_sm_is_tx_fifo_full(g_pio, g_sm)) {
        pio_sm_put(g_pio, g_sm, 0);
    }

    /* invert because buttons are connected to GND */
    uint32_t raw = pio_sm_get_blocking(g_pio, g_sm);
    uint16_t data = (uint16_t) (raw & 0xffff);
    data = (uint16_t) ~data;

    joy1->state = 0x00;
    joy2->state = 0x00;

    for (unsigned i = 0; i < 8; ++i) {
        uint8_t pair = (data >> (i * 2)) & 0x03;

        if (pair & 1) {
            joy1->state |= (1 << i);
        }

        if (pair & 2) {
            joy2->state |= (1 << i);
        }
    }
}
