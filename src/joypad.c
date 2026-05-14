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

/**
 * @file joypad.c
 * @brief Simple emulation of the NES joypad (controller).
 *
 * This code emulates the CD4021 shift register used in the NES controller.
 * The console uses a STROBE signal to lock the button states, and then
 * reads the 8 buttons one by one.
 */

#include <fwkes/joypad.h>
#include <string.h>

/**
 * @brief Resets the joypad state to zero.
 *
 * This function clears everything in the joypad object.
 * It makes all variables equal to zero.
 *
 * @param self Pointer to the Joypad object.
 */
void joypad_reset(Joypad *self) { memset(self, 0, sizeof(*self)); }

/**
 * @brief Writes data to the joypad to control the STROBE signal.
 *
 * The NES console writes to the controller to lock the button states.
 * Only the lowest bit (bit 0) of the data is used for the strobe.
 * If strobe is 1 (ON), the button index resets to 0. This means it will 
 * keep reading the first button.
 *
 * @param self Pointer to the Joypad object.
 * @param data Data byte from the console.
 */
void joypad_write(Joypad *self, uint8_t data) {
    self->strobe = data & 1;

    if (self->strobe) {
        self->btn_idx = 0;
    }
}

/**
 * @brief Reads one button bit from the joypad.
 *
 * The NES reads the 8 buttons one by one (A, B, Select, Start, Up, Down, Left, Right).
 * If we already read all 8 buttons, it will just return 1.
 * If the strobe is 0 (OFF), we move to the next button for the next time we read.
 *
 * @param self Pointer to the Joypad object.
 * @return Returns 0 or 1 for the current button. Returns 1 if all buttons are read.
 */
uint8_t joypad_read(Joypad *self) {
    if (self->btn_idx >= 8) {
        return 1;
    }

    uint8_t value = (self->state & (1 << self->btn_idx)) >> self->btn_idx;

    if (!self->strobe && self->btn_idx < 8) {
        ++self->btn_idx;
    }

    return value;
}

/**
 * @brief Looks at the current button bit, but does not move to the next one.
 *
 * This is exactly like joypad_read(), but it does not change the button index.
 * It is very good for debugging because it does not change the state of the emulator.
 *
 * @param self Pointer to the Joypad object.
 * @return Returns 0 or 1 for the current button. Returns 1 if all buttons are read.
 */
uint8_t joypad_peek(const Joypad *self) {
    if (self->btn_idx >= 8) {
        return 1;
    }

    return (self->state & (1 << self->btn_idx)) >> self->btn_idx;
}
