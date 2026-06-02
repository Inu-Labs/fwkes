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
 * @file trace.h
 * @brief Trace logging subsystem.
 *
 * Lightweight diagnostic tracing facility designed for
 * use in both embedded (RP2350) and desktop host builds.
 *
 * Each trace call has assigned category, that maps to logical subsystem, e.g.
 * CPU, PPU, bus, etc.; and a **type** (error, warning, ...). It's also possible
 * to filter (ignore) messages based on category or type.
 *
 * @par Compile-time switches
 *
 * In order to use tracing facilities, `TRACE_ON` must be defined. To enable
 * utility functions like trace_cpu(), trace_ppu(), etc., there are macro
 * switches for each individual component category.
 *
 *  | Macro            | Effect                                              |
 *  |------------------|-----------------------------------------------------|
 *  | `TRACE_ON`       | Enables tracing.                                    |
 *  | `TRACE_BUS_ON`   | Enables `trace_bus()` (requires `TRACE_ON`).        |
 *  | `TRACE_CPU_ON`   | Enables `trace_cpu()` (requires `TRACE_ON`).        |
 *  | `TRACE_PPU_ON`   | Enables `trace_ppu()` (requires `TRACE_ON`).        |
 *  | `TRACE_FWX_ON`   | Enables `trace_fwx()` (requires `TRACE_ON`).        |
 *
 * @par Runtime filtering
 *
 *  Category and type filters are bitmasks maintained internally.  By default
 *  all categories and types are allowed through.  Use
 *  @ref trace_add_cat_filter / @ref trace_remove_cat_filter and
 *  @ref trace_add_type_filter / @ref trace_remove_type_filter to narrow the
 *  output at runtime without recompiling.
 *
 * @par Output stream
 *
 * The output destination by default goes to `stdout`, but it can be set to
 * different output stream using trace_set_stream(). Alternatively, one can
 * redefine macro `DEFAULT_TRACE_STREAM`, although it requires recompiling trace
 * source files.
 */

#pragma once

#include <stdarg.h>
#include <stdio.h>

/**
 * @def DEFAULT_TRACE_STREAM
 * @brief Default `FILE *` stream used for trace output.
 */
#ifndef DEFAULT_TRACE_STREAM
#    define DEFAULT_TRACE_STREAM stdout
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Extracts the filename from a full path.
 *
 * Given an absolute or relative file path (e.g. from `__FILE__`
 * macro), returns a pointer to the last path component - i.e. the filename
 * without any leading directory segments.
 *
 * This is used internally by the @ref trace and related macros to keep log
 * output compact.  Callers should not need to invoke it directly.
 *
 * @param path  Full file path string, e.g. `"/home/user/fwkes/src/cpu.c"`.
 * @return      Pointer into @p path at the start of the filename, e.g.
 *              `"cpu.c"`.  The returned pointer shares storage with @p path
 *              and must not be freed.
 */
const char *trace__get_filename(const char *path);

/**
 * @brief Logical subsystem categories for trace messages.
 *
 * Can be combined in bitmasks to assign multiple categories for a message.
 */
typedef enum TraceCategory {
    TRACE_GENERAL = 1u << 0, /**< General / uncategorised messages.          */
    TRACE_BUS = 1u << 1,     /**< Memory bus read/write transactions.        */
    TRACE_CPU = 1u << 2,     /**< CPU execution, registers, and state.       */
    TRACE_PPU = 1u << 3,     /**< PPU rendering pipeline events.             */
    TRACE_FWX = 1u << 4      /**< Firmware extension protocol events.        */
} TraceCategory;

/**
 * @brief Event-type tags for trace messages.
 *
 * Can be combined in bitmasks to assign multiple tags for a message.
 */
typedef enum TraceType {
    TRACE_MSG = 1u << 0,       /**< Generic informational message.         */
    TRACE_ERROR = 1u << 1,     /**< Error condition.                       */
    TRACE_WARN = 1u << 2,      /**< Unexpected but recoverable condition.  */
    TRACE_REG_READ = 1u << 3,  /**< Hardware register read.                */
    TRACE_REG_WRITE = 1u << 4, /**< Hardware register write.               */
    TRACE_W_LATCH = 1u << 5,   /**< Write-latch state change.              */
    TRACE_VBLANK = 1u << 6,    /**< Vertical blank interval started.       */
    TRACE_NMI = 1u << 7,       /**< Non-maskable interrupt asserted.       */
    TRACE_0_HIT = 1u << 8,     /**< PPU sprite-zero hit.                   */

    TRACE_START_COND = 1u << 9, /**< FWX START condition on the bus.       */
    TRACE_STOP_COND = 1u << 10, /**< FWX STOP condition on the bus.        */
    TRACE_CLK_EDGE = 1u << 11,  /**< FWX clock edge event.                 */
    TRACE_TX_DATA = 1u << 12,   /**< FWX byte transmitted.                 */
    TRACE_RX_DATA = 1u << 13    /**< FWX byte received.                    */
} TraceType;

/**
 * @brief Emits a formatted trace message (internal implementation).
 *
 * This is the underlying implementation called by all category-specific
 * macros.  Prefer the macros (@ref trace, @ref trace_bus, …) over calling
 * this function directly, as the macros automatically capture `__LINE__` and
 * `__FILE__` and can be compiled out entirely when tracing is disabled.
 *
 * The message is only written to the output stream if both @p cat and
 * @p type pass the current runtime filters (see @ref trace_add_cat_filter and
 * @ref trace_add_type_filter).
 *
 * @param cat   Subsystem category @ref TraceCategory.
 * @param line  Source line number.
 * @param file  Source filename.
 * @param type  Event type @ref TraceType.
 * @param fmt   `printf`-style format string.
 * @param ...   Format arguments matching @p fmt.
 */
void trace_(
    TraceCategory cat, int line, const char *file, TraceType type,
    const char *fmt, ...
);

/**
 * @brief `va_list` variant of @ref trace_.
 *
 * Identical to @ref trace_ but accepts a `va_list` instead of variadic
 * arguments.
 *
 * @param cat   Subsystem category @ref TraceCategory.
 * @param line  Source line number.
 * @param file  Source filename.
 * @param type  Event type @ref TraceType.
 * @param fmt   `printf`-style format string.
 * @param ...   Format arguments matching @p fmt.
 * @param vargs  Argument list initialised by the caller with `va_start`.
 *               The caller is responsible for calling `va_end` afterwards.
 */
void vtrace(
    TraceCategory cat, int line, const char *file, TraceType type,
    const char *fmt, va_list vargs
);

/**
 * @brief Set output stream for all trace messages.
 *
 * @param stream  An open, writable `FILE *`.  Passing `NULL` has undefined
 *                behaviour; use @ref DEFAULT_TRACE_STREAM to restore the
 *                default.
 */
void trace_set_stream(FILE *stream);

/**
 * @brief Add a category to the category blacklist.
 *
 * After this call, messages tagged with @p cat will be suppressed.
 *
 * @param cat  Category bit to enable.
 */
void trace_add_cat_filter(TraceCategory cat);

/**
 * @brief Remove a category from the category blacklist.
 *
 * After this call, messages tagged with @p cat will be allowed.
 *
 * @param cat  Category bit to disable.
 */
void trace_remove_cat_filter(TraceCategory cat);

/**
 * @brief Add a event-type tag to the category blacklist.
 *
 * After this call, messages tagged with @p type will be suppressed.
 *
 * @param type  Tag bit to enable.
 */
void trace_add_type_filter(TraceType type);

/**
 * @brief Remove a event-type tag to the category blacklist.
 *
 * After this call, messages tagged with @p type will be allowed.
 *
 * @param type  Tag bit to enable.
 */
void trace_remove_type_filter(TraceType type);

/**
 * @brief Resets the category filter to its default state (all categories
 * allowed).
 */
void trace_reset_cat_filter(void);

/**
 * @brief Resets the type filter to its default state (all types allowed).
 */
void trace_reset_type_filter(void);

/**
 * @def trace(type, fmt, ...)
 * @brief Emits a @ref TRACE_GENERAL trace message.
 *
 * Automatically captures the current source file and line number.
 * Compiled out entirely when `TRACE_ON` is not defined.
 *
 * @param type  @ref TraceType value.
 * @param fmt   `printf`-style format string.
 */
#ifdef TRACE_ON

#    define trace(...)                                                         \
        trace_(                                                                \
            TRACE_GENERAL, __LINE__, trace__get_filename(__FILE__),            \
            __VA_ARGS__                                                        \
        )

/**
 * @def trace_bus(type, fmt, ...)
 * @brief Emits a @ref TRACE_BUS trace message.
 *
 * Requires both `TRACE_ON` and `TRACE_BUS_ON` to be defined at compile time;
 * otherwise expands to nothing.
 *
 * @param type  @ref TraceType value.
 * @param fmt   `printf`-style format string.
 */
#    ifdef TRACE_BUS_ON
#        define trace_bus(...)                                                 \
            trace_(                                                            \
                TRACE_BUS, __LINE__, trace__get_filename(__FILE__),            \
                __VA_ARGS__                                                    \
            )
#    else
#        define trace_bus(...)
#    endif

/**
 * @def trace_cpu(type, fmt, ...)
 * @brief Emits a @ref TRACE_CPU trace message.
 *
 * Requires both `TRACE_ON` and `TRACE_CPU_ON` to be defined at compile time;
 * otherwise expands to nothing.
 *
 * @param type  @ref TraceType value.
 * @param fmt   `printf`-style format string.
 */
#    ifdef TRACE_CPU_ON
#        define trace_cpu(...)                                                 \
            trace_(                                                            \
                TRACE_CPU, __LINE__, trace__get_filename(__FILE__),            \
                __VA_ARGS__                                                    \
            )
#    else
#        define trace_cpu(...)
#    endif

/**
 * @def trace_ppu(type, fmt, ...)
 * @brief Emits a @ref TRACE_PPU trace message.
 *
 * Requires both `TRACE_ON` and `TRACE_PPU_ON` to be defined at compile time;
 * otherwise expands to nothing.
 *
 * @param type  @ref TraceType value.
 * @param fmt   `printf`-style format string.
 * @param ...   Format arguments.
 */
#    ifdef TRACE_PPU_ON
#        define trace_ppu(...)                                                 \
            trace_(                                                            \
                TRACE_PPU, __LINE__, trace__get_filename(__FILE__),            \
                __VA_ARGS__                                                    \
            )
#    else
#        define trace_ppu(...)
#    endif

/**
 * @def trace_fwx(type, fmt, ...)
 * @brief Emits a @ref TRACE_FWX trace message.
 *
 * Requires both `TRACE_ON` and `TRACE_FWX_ON` to be defined at compile time;
 * otherwise expands to nothing.
 *
 * @param type  @ref TraceType value.
 * @param fmt   `printf`-style format string.
 */
#    ifdef TRACE_FWX_ON
#        define trace_fwx(...)                                                 \
            trace_(                                                            \
                TRACE_FWX, __LINE__, trace__get_filename(__FILE__),            \
                __VA_ARGS__                                                    \
            )
#    else
#        define trace_fwx(...)
#    endif

#else
#    define trace(...)
#    define trace_bus(...)
#    define trace_cpu(...)
#    define trace_ppu(...)
#    define trace_fwx(...)
#endif

#ifdef __cplusplus
}
#endif
