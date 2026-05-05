/*
 * Copyright (C) 2025-present inunix3, kubaa-ma
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "../joypad.h"

/* OUT pins have to be close to each other, otherwise PIO will not work. */
#define JOYPAD1_OUT_PIN 0
#define JOYPAD2_OUT_PIN 1
#define JOYPAD_CLK_PIN 10
#define JOYPAD_STR_PIN 11

void init_joypads(void);
void update_joypads(Joypad *joy1, Joypad *joy2);
