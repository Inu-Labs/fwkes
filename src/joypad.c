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

#include <fwkes/joypad.h>
#include <string.h>

void joypad_reset(Joypad *self) { memset(self, 0, sizeof(*self)); }

void joypad_write(Joypad *self, uint8_t data) {
    self->strobe = data & 1;

    if (self->strobe) {
        self->btn_idx = 0;
    }
}

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

uint8_t joypad_peek(const Joypad *self) {
    if (self->btn_idx >= 8) {
        return 1;
    }

    return (self->state & (1 << self->btn_idx)) >> self->btn_idx;
}
