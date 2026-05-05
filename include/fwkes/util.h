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

#include <stdint.h>
#include <inttypes.h>

#ifdef BUILD_RP2350
#    include <pico/stdlib.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define FORCE_INLINE static inline __attribute__((always_inline))

#ifdef BUILD_RP2350
#    define NOTFLASH_FN(name) __not_in_flash_func(name)
#else
#    define NOTFLASH_FN(name) name
#endif

#ifdef BUILD_RP2350
typedef uint32_t CycleCounter;
typedef int32_t CycleDiff;
#define CYCLE_COUNTER_PRIu PRIu32
#else
typedef uint64_t CycleCounter;
typedef int64_t CycleDiff;
#define CYCLE_COUNTER_PRIu PRIu64
#endif

#ifdef __cplusplus
}
#endif
