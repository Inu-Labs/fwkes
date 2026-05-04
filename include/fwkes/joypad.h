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

#pragma once

#include <stdbool.h>
#include <stdint.h>

#define CONTROLLER_POLL_ADDR 0x4016
#define CONTROLLER1_ADDR 0x4016
#define CONTROLLER2_ADDR 0x4017

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BTN_A = 1 << 0,
    BTN_B = 1 << 1,
    BTN_SELECT = 1 << 2,
    BTN_START = 1 << 3,
    BTN_UP = 1 << 4,
    BTN_DOWN = 1 << 5,
    BTN_LEFT = 1 << 6,
    BTN_RIGHT = 1 << 7
} Button;

typedef struct Joypad {
#ifdef BUILD_RP2350
    uint8_t data;
#endif

    uint8_t btn_idx;
    uint8_t state;
    bool strobe;
} Joypad;

void joypad_reset(Joypad *self);
void joypad_write(Joypad *self, uint8_t data);
uint8_t joypad_read(Joypad *self);
uint8_t joypad_peek(const Joypad *self);

#ifdef __cplusplus
}
#endif
