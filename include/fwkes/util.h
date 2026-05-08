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
 * @file util.h
 * @brief Utility macros, attributes, and platform-specific type definitions.
 *
 * This header provides:
 *
 *  - @ref FORCE_INLINE  - compiler hint to always inline a function.
 *  - @ref NOTFLASH_FN   - attribute that places a function in RAM rather than flash (RP2350 only;
 *      no-op elsewhere).
 *  - @ref CycleCounter  - unsigned integer type wide enough to count emulator cycles.
 *  - @ref CycleDiff     - signed counterpart of CycleCounter, suitable for
 *                         expressing the difference between two cycle timestamps.
 */

#include <stdint.h>

#ifdef BUILD_RP2350
#    include <pico/stdlib.h>
#endif

/* -------------------------------------------------------------------------
 * Compiler attribute helpers
 * ---------------------------------------------------------------------- */

/**
 * @def FORCE_INLINE
 * @brief Decorator that hints the compiler to always inline the function.
 *
 * This function should be used with care: in some situations inlining can improve performance, while
 * in some cases it either makes no difference, or makes performance worse (although the same applies
 * to standard @c inline).
 *
 * @par Example
 * @code
 *  FORCE_INLINE uint8_t clamp_u8(int v) {
 *      return (uint8_t)(v < 0 ? 0 : v > 255 ? 255 : v);
 *  }
 * @endcode
 */
#define FORCE_INLINE static inline __attribute__((always_inline))

/* -------------------------------------------------------------------------
 * Memory-placement helpers
 * ---------------------------------------------------------------------- */

/**
 * @def NOTFLASH_FN(name)
 * @brief Decorator that instructs compiler to place function code into RAM instead of Flash (RP2350
 * only).
 *
 * This is done by telling compiler (linker) to place the code to `.time_critical` section in the
 * final binary file.
 *
 * This can be useful for performance-critical code (SRAM is faster than Flash), or executing code
 * while writing to Flash, since XIP cache gets blocked.
 *
 * On other platforms this just expands to @p name.
 *
 * @param name Function name.
 *
 * @par Example
 * @code
 *  void NOTFLASH_FN(foo)(int a, int b) {
 *      // e.g. performance-critical code...
 *  }
 * @endcode
 */

#ifdef BUILD_RP2350
#    define NOTFLASH_FN(name) __not_in_flash_func(name)
#else
#    define NOTFLASH_FN(name) name
#endif

/* -------------------------------------------------------------------------
 * Cycle-counter types
 * ---------------------------------------------------------------------- */

/**
 * @defgroup emu_cycle_counting Emulator cycle counting types
 *
 * For working with emulation cycle counters and timestamps, this API provides following types:
 *
 * - Unsigned @ref CycleCounter type for counting cycles.
 * - Signed @ref CycleDiff type for calculating difference between two cycle timestamps.
 *
 * These types have the same size as platform's word size for performance reasons, since cycle
 * counters are typically accessed frequently.
 *
 * On the RP2350 backend, these types are 32-bit wide. On the desktop backend, they are 64-bit wide.
 *
 * @{
 */

/**
 * @typedef CycleCounter
 * @brief Unsigned type suitable for counting emulation cycles.
 */

/**
 * @typedef CycleDiff
 * @brief Signed type for the difference between two @ref CycleCounter values.
 */

#ifdef BUILD_RP2350
typedef uint32_t CycleCounter;
typedef int32_t CycleDiff;
#else
typedef uint64_t CycleCounter;
typedef int64_t CycleDiff;
#endif

/** @} */
