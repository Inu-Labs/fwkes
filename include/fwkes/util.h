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
 * @file util.h
 * @brief Utility macros, attributes, and platform-specific type definitions.
 *
 * This header centralizes low-level portability helpers used across the
 * codebase. It abstracts away compiler-specific attributes and
 * platform-specific integer widths so that the rest of the code can be
 * written in a target-agnostic way.
 *
 * Specifically, this header provides:
 *  - @ref FORCE_INLINE  – compiler hint to always inline a function.
 *  - @ref NOTFLASH_FN   – attribute that places a function in RAM rather
 *                         than flash (RP2350 only; no-op elsewhere).
 *  - @ref CycleCounter  – unsigned integer type wide enough to hold a
 *                         hardware cycle-counter value on the current target.
 *  - @ref CycleDiff     – signed counterpart of CycleCounter, suitable for
 *                         expressing the difference between two timestamps.
 *
 * @par Supported platforms
 *  | Macro defined   | Target                        |
 *  |-----------------|-------------------------------|
 *  | BUILD_RP2350    | Raspberry Pi RP2350 (Pico SDK)|
 *  | *(none)*        | Generic desktop / host build  |
 *
 * @par Usage example
 * @code
 *  #include "util.h"
 *
 *  // Always inlined helper – zero call overhead in hot loops.
 *  FORCE_INLINE uint32_t saturate(uint32_t v, uint32_t max) {
 *      return v > max ? max : v;
 *  }
 *
 *  // Placed in RAM so it can safely run while flash is being erased.
 *  void NOTFLASH_FN(flash_safe_isr)(void) { ... }
 *
 *  // Portable timing measurement.
 *  CycleCounter t0 = read_cycle_counter();
 *  do_work();
 *  CycleDiff elapsed = (CycleDiff)(read_cycle_counter() - t0);
 * @endcode
 */

#pragma once

#include <stdint.h>

#ifdef BUILD_RP2350
#    include <pico/stdlib.h>
#endif

/* -------------------------------------------------------------------------
 * Compiler attribute helpers
 * ---------------------------------------------------------------------- */

/**
 * @def FORCE_INLINE
 * @brief Decorator that instructs the compiler to always inline the function.
 *
 * Combining `static`, `inline`, and GCC/Clang's `__attribute__((always_inline))`
 * guarantees that the function body is substituted at every call site,
 * regardless of the optimisation level or the compiler's own inlining
 * heuristics.
 *
 * @par When to use
 *  - Tiny, performance-critical helpers called in tight loops.
 *  - Functions whose call overhead would measurably affect timing.
 *  - Wrappers around a single instruction or intrinsic.
 *
 * @par When *not* to use
 *  - Large functions – forced inlining inflates code size and can hurt
 *    I-cache performance.
 *  - Functions that are only called once – the compiler already inlines
 *    those automatically.
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

#ifdef BUILD_RP2350
/**
 * @def NOTFLASH_FN(name)
 * @brief Places a function into SRAM instead of flash memory (RP2350 only).
 *
 * On the RP2350 the default code storage is the external QSPI flash, which
 * is fetched through an XIP (execute-in-place) cache. While this is
 * transparent most of the time, it creates two problems:
 *
 *  1. **Flash-unsafe operations** – certain operations (e.g. erasing or
 *     programming flash) temporarily disable XIP.  Any code still executing
 *     from flash at that moment will hard-fault.  Moving such code to SRAM
 *     via this macro avoids the hazard.
 *
 *  2. **Deterministic latency** – XIP cache misses introduce variable
 *     latency that makes cycle-accurate timing difficult.  SRAM execution
 *     has fixed, predictable access time.
 *
 * Internally this macro expands to the Pico SDK helper
 * `__not_in_flash_func()`, which applies the linker section attribute
 * `.time_critical.<name>` and causes the function to be copied to SRAM by
 * the startup code.
 *
 * @note  The function body must **not** call any function that itself lives
 *        in flash (unless that function is also marked NOTFLASH_FN or is
 *        otherwise guaranteed to reside in SRAM).
 *
 * @param name  The bare (unquoted) function name, e.g. `my_isr`.
 *              Use it as: `void NOTFLASH_FN(my_isr)(void) { ... }`
 *
 * @par Example
 * @code
 *  // Safe to call from a flash-erase context or a hard-IRQ handler.
 *  void NOTFLASH_FN(critical_timer_isr)(void) {
 *      gpio_put(LED_PIN, 1);
 *  }
 * @endcode
 */
#    define NOTFLASH_FN(name) __not_in_flash_func(name)

#else
/**
 * @def NOTFLASH_FN(name)
 * @brief No-op fallback for non-RP2350 targets.
 *
 * On desktop / host builds there is no flash-vs-RAM distinction, so this
 * macro expands to the function name unchanged.  Code that uses
 * `NOTFLASH_FN` therefore compiles and links correctly on all targets
 * without any `#ifdef` guards at the call site.
 *
 * @param name  The bare function name (passed through unmodified).
 */
#    define NOTFLASH_FN(name) name
#endif

/* -------------------------------------------------------------------------
 * Cycle-counter types
 * ---------------------------------------------------------------------- */

#ifdef BUILD_RP2350
/**
 * @typedef CycleCounter
 * @brief Unsigned type that holds a raw hardware cycle-counter value (RP2350).
 *
 * On the RP2350 the cycle counter is a 32-bit register (accessible via the
 * Cortex-M33 DWT_CYCCNT peripheral).  Using a 32-bit type avoids the extra
 * instructions that 64-bit arithmetic would generate on a 32-bit core.
 *
 * @note  The counter wraps around after 2^32 cycles (~4 seconds at 150 MHz).
 *        Always compute differences with @ref CycleDiff to handle wrap-around
 *        correctly via two's-complement subtraction.
 */
typedef uint32_t CycleCounter;

/**
 * @typedef CycleDiff
 * @brief Signed type for the difference between two @ref CycleCounter values (RP2350).
 *
 * Subtracting two `CycleCounter` values and storing the result in a
 * `CycleDiff` correctly handles counter wrap-around as long as the measured
 * interval is shorter than 2^31 cycles (~14 seconds at 150 MHz).
 *
 * @par Example
 * @code
 *  CycleCounter start = dwt_get_cycle_count();
 *  do_work();
 *  CycleDiff elapsed = (CycleDiff)(dwt_get_cycle_count() - start);
 *  // elapsed is correct even if the counter wrapped around once.
 * @endcode
 */
typedef int32_t CycleDiff;

#else
/**
 * @typedef CycleCounter
 * @brief Unsigned type that holds a raw hardware cycle-counter value (generic).
 *
 * On 64-bit desktop targets (Linux, macOS, Windows) performance counters are
 * typically 64-bit values (e.g. `clock_gettime(CLOCK_MONOTONIC)` nanoseconds,
 * or the x86 `RDTSC` instruction).  Using a 64-bit type avoids lossy
 * truncation and extends the wrap-around period to ~584 years at 1 GHz.
 */
typedef uint64_t CycleCounter;

/**
 * @typedef CycleDiff
 * @brief Signed type for the difference between two @ref CycleCounter values (generic).
 *
 * The signed 64-bit type can represent differences up to ~292 years worth of
 * nanoseconds, which is sufficient for any practical measurement on a desktop
 * host.
 */
typedef int64_t CycleDiff;
#endif
