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

/**
 * @file joypad.h
 * @brief Standard NES controller.
 *
 * Original NES controllers were using internally CD4021 shift registers, so
 * basically, we're emulating behavior of the shift register.
 */

#include <stdbool.h>
#include <stdint.h>

/**
 * @def CONTROLLER_POLL_ADDR
 * @brief Readable address common for both joypads.
 */
#define CONTROLLER_POLL_ADDR 0x4016

/**
 * @def CONTROLLER1_ADDR
 * @brief Writable address of the primary joypad.
 */
#define CONTROLLER1_ADDR 0x4016
/**
 * @def CONTROLLER2_ADDR
 * @brief Writable address of the secondary joypad.
 */
#define CONTROLLER2_ADDR 0x4017

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief %Button bit masks.
 */
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

/**
 * @brief State of emulated joypad. In fact, it's representation of a CD4021
 * shift register.
 */
typedef struct Joypad {
#ifdef BUILD_RP2350
    uint8_t data;
#endif

    uint8_t btn_idx;
    uint8_t state;
    bool strobe;
} Joypad;

/**
 * @brief Reset state of joypad.
 *
 * All fields are set to 0.
 *
 * @param self Joypad instance.
 */
void joypad_reset(Joypad *self);
/**
 * @brief MMIO write interface.
 *
 * The first bit sets the strobe mode. When its 1, shift register is reloaded
 * and consequent reads will return only staet of the first button. When its 0,
 * shift register returns state of the current buttons and shift to the next
 * one.
 *
 * Remaining 7 bits have no meaning and are ignored.
 *
 * @param self Joypad instance.
 * @param value Strobe mode toggle.
 */
void joypad_write(Joypad *self, uint8_t data);
/**
 * @brief MMIO read interface with side effects.
 *
 * Every reading returns state of the current button. When strobe mode is
 * enabled (see @ref joypad_write()), only the first button is read. If strobe
 * mode is disabled, each read will return state of the current button and move
 * to the next button (**side-effect**). When 8 buttons are read, it returns 1
 * until it's reloaded by enabling strobe mode.
 *
 * @param self Joypad instance.
 * @return State of the current button.
 */
uint8_t joypad_read(Joypad *self);
/**
 * @brief MMIO read interface without side effects.
 *
 * Every reading returns state of the current button. When strobe mode is
 * enabled (see @ref joypad_write()), only the first button is read. If strobe
 * mode is disabled, each read will return state of the current button. When 8
 * buttons are read, it returns 1 until it's reloaded by enabling strobe mode.
 *
 * @param self Joypad instance.
 * @return State of the current button.
 */
uint8_t joypad_peek(const Joypad *self);

#ifdef __cplusplus
}
#endif
