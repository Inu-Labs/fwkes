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

#include <pico/stdlib.h>

#define LED_GENERAL_PIN 20
#define LED_ERROR_PIN 21

#define led_general_set(v) gpio_put(LED_GENERAL_PIN, v)
#define led_general_on() led_general_set(1)
#define led_general_off() led_general_set(0)
#define led_error_set(v) gpio_put(LED_ERROR_PIN, v)
#define led_error_on() led_error_set(1)
#define led_error_off() led_error_set(0)
