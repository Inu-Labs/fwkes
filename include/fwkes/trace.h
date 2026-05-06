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
 * @brief Structured, filterable trace/logging subsystem.
 *
 * This header exposes a lightweight diagnostic tracing facility designed for
 * use in both embedded (RP2350) and desktop host builds.  Every trace call
 * carries two orthogonal tags – a @ref TraceCategory and a @ref TraceType –
 * which together allow fine-grained, runtime filtering of trace output.
 *
 * @par Design overview
 *  - **Categories** map to logical hardware/software subsystems (CPU, PPU,
 *    bus, …).  Each category can be independently enabled or disabled at
 *    compile time (see below) and further filtered at runtime.
 *  - **Types** describe the nature of the event (error, warning, register
 *    access, interrupt, …) and can also be filtered at runtime.
 *  - All output is written to a configurable `FILE *` stream, defaulting to
 *    `stdout`.  On embedded targets this can be redirected to a UART or a
 *    ring-buffer backed stream.
 *
 * @par Compile-time switches
 *  | Macro            | Effect                                              |
 *  |------------------|-----------------------------------------------------|
 *  | `TRACE_ON`       | Master switch – enables all `trace()` calls.        |
 *  | `TRACE_BUS_ON`   | Enables `trace_bus()` (requires `TRACE_ON`).        |
 *  | `TRACE_CPU_ON`   | Enables `trace_cpu()` (requires `TRACE_ON`).        |
 *  | `TRACE_PPU_ON`   | Enables `trace_ppu()` (requires `TRACE_ON`).        |
 *  | `TRACE_FWX_ON`   | Enables `trace_fwx()` (requires `TRACE_ON`).        |
 *
 *  When a switch is **not** defined the corresponding macro expands to
 *  nothing, so there is zero runtime cost in release builds.
 *
 * @par Runtime filtering
 *  Category and type filters are bitmasks maintained internally.  By default
 *  all categories and types are allowed through.  Use
 *  @ref trace_add_cat_filter / @ref trace_remove_cat_filter and
 *  @ref trace_add_type_filter / @ref trace_remove_type_filter to narrow the
 *  output at runtime without recompiling.
 *
 * @par Output stream
 *  The default output stream is controlled by the @ref DEFAULT_TRACE_STREAM
 *  macro (defaults to `stdout`).  It can be changed at runtime via
 *  @ref trace_set_stream.
 *
 * @par Usage example
 * @code
 *  // Redirect all output to stderr.
 *  trace_set_stream(stderr);
 *
 *  // Show only errors and warnings globally.
 *  trace_reset_type_filter();
 *  trace_add_type_filter(TRACE_ERROR);
 *  trace_add_type_filter(TRACE_WARN);
 *
 *  // Emit a general informational message.
 *  trace(TRACE_MSG, "Initialised mapper %d", mapper_id);
 *
 *  // Emit a PPU register-write trace (compiled out unless TRACE_PPU_ON).
 *  trace_ppu(TRACE_REG_WRITE, "PPUCTRL <- 0x%02X", value);
 * @endcode
 */

#pragma once

#include <stdarg.h>
#include <stdio.h>

/**
 * @def DEFAULT_TRACE_STREAM
 * @brief Default `FILE *` stream used for trace output.
 *
 * Override this macro at compile time to redirect all trace output to a
 * different stream (e.g. `stderr` or a custom UART-backed `FILE`).  The
 * active stream can also be changed at runtime with @ref trace_set_stream.
 *
 * @note  On embedded targets with no filesystem, a custom `FILE`
 *        implementation backed by a circular buffer or DMA FIFO is a common
 *        approach.
 */
#ifndef DEFAULT_TRACE_STREAM
#    define DEFAULT_TRACE_STREAM stdout
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Extracts the bare filename from a full path.
 *
 * Given an absolute or relative file path (as produced by the `__FILE__`
 * macro), returns a pointer to the last path component – i.e. the filename
 * without any leading directory segments.
 *
 * This is used internally by the @ref trace and related macros to keep log
 * output compact.  Callers should not need to invoke it directly.
 *
 * @param path  Full file path string, e.g. `"/home/user/src/cpu.c"`.
 * @return      Pointer into @p path at the start of the filename, e.g.
 *              `"cpu.c"`.  The returned pointer shares storage with @p path
 *              and must not be freed.
 */
const char *trace__get_filename(const char *path);

/* -------------------------------------------------------------------------
 * Enumerations
 * ---------------------------------------------------------------------- */

/**
 * @brief Logical subsystem categories for trace messages.
 *
 * Each enumerator is a distinct power-of-two bit so that multiple categories
 * can be combined into a bitmask for filtering purposes.
 *
 * | Enumerator      | Subsystem                                    |
 * |-----------------|----------------------------------------------|
 * | TRACE_GENERAL   | General / uncategorised messages.            |
 * | TRACE_BUS       | Memory / address bus transactions.           |
 * | TRACE_CPU       | CPU instruction execution and state.         |
 * | TRACE_PPU       | Picture Processing Unit rendering pipeline.  |
 * | TRACE_FWX       | Firmware extension / protocol layer (FWX).   |
 */
typedef enum TraceCategory {
    TRACE_GENERAL = 1u << 0, /**< General / uncategorised messages.          */
    TRACE_BUS     = 1u << 1, /**< Memory bus read/write transactions.        */
    TRACE_CPU     = 1u << 2, /**< CPU execution, registers, and state.       */
    TRACE_PPU     = 1u << 3, /**< PPU rendering pipeline events.             */
    TRACE_FWX     = 1u << 4  /**< Firmware extension protocol events.        */
} TraceCategory;

/**
 * @brief Event-type tags for trace messages.
 *
 * Each enumerator is a distinct power-of-two bit so that multiple types can
 * be combined into a bitmask for filtering purposes.
 *
 * The first group of types (TRACE_MSG … TRACE_0_HIT) is general and applies
 * to any @ref TraceCategory.  The second group (TRACE_START_COND …
 * TRACE_RX_DATA) is specific to the FWX firmware-extension protocol and is
 * primarily used with @ref TRACE_FWX category traces.
 *
 * | Enumerator       | Meaning                                            |
 * |------------------|----------------------------------------------------|
 * | TRACE_MSG        | Informational message (default for generic traces).|
 * | TRACE_ERROR      | Non-fatal error condition.                         |
 * | TRACE_WARN       | Warning: unexpected but recoverable condition.     |
 * | TRACE_REG_READ   | Hardware register read access.                     |
 * | TRACE_REG_WRITE  | Hardware register write access.                    |
 * | TRACE_W_LATCH    | Write-latch toggled (double-write registers).      |
 * | TRACE_VBLANK     | Vertical blank interval started.                   |
 * | TRACE_NMI        | Non-maskable interrupt asserted.                   |
 * | TRACE_0_HIT      | Sprite-zero hit detected by the PPU.               |
 * | TRACE_START_COND | FWX: START condition detected on the bus.          |
 * | TRACE_STOP_COND  | FWX: STOP condition detected on the bus.           |
 * | TRACE_CLK_EDGE   | FWX: Clock edge event.                             |
 * | TRACE_TX_DATA    | FWX: Data byte transmitted.                        |
 * | TRACE_RX_DATA    | FWX: Data byte received.                           |
 */
typedef enum TraceType {
    /* --- General types -------------------------------------------------- */
    TRACE_MSG       = 1u << 0,  /**< Generic informational message.          */
    TRACE_ERROR     = 1u << 1,  /**< Non-fatal error condition.              */
    TRACE_WARN      = 1u << 2,  /**< Unexpected but recoverable condition.   */
    TRACE_REG_READ  = 1u << 3,  /**< Hardware register read.                 */
    TRACE_REG_WRITE = 1u << 4,  /**< Hardware register write.               */
    TRACE_W_LATCH   = 1u << 5,  /**< Write-latch state change.              */
    TRACE_VBLANK    = 1u << 6,  /**< Vertical blank interval started.       */
    TRACE_NMI       = 1u << 7,  /**< Non-maskable interrupt asserted.       */
    TRACE_0_HIT     = 1u << 8,  /**< PPU sprite-zero hit.                   */

    /* --- FWX protocol types --------------------------------------------- */
    TRACE_START_COND = 1u << 9,  /**< FWX START condition on the bus.       */
    TRACE_STOP_COND  = 1u << 10, /**< FWX STOP condition on the bus.        */
    TRACE_CLK_EDGE   = 1u << 11, /**< FWX clock edge event.                 */
    TRACE_TX_DATA    = 1u << 12, /**< FWX byte transmitted.                 */
    TRACE_RX_DATA    = 1u << 13  /**< FWX byte received.                    */
} TraceType;

/* -------------------------------------------------------------------------
 * Core trace functions
 * ---------------------------------------------------------------------- */

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
 * @param cat   Subsystem category – one of @ref TraceCategory.
 * @param line  Source line number (pass `__LINE__`).
 * @param file  Source filename, ideally the bare name returned by
 *              @ref trace__get_filename.
 * @param type  Event type – one of @ref TraceType.
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
 * arguments.  Useful when implementing wrapper functions that forward
 * variadic arguments without unpacking them.
 *
 * @param cat    Subsystem category – one of @ref TraceCategory.
 * @param line   Source line number.
 * @param file   Source filename.
 * @param type   Event type – one of @ref TraceType.
 * @param fmt    `printf`-style format string.
 * @param vargs  Argument list initialised by the caller with `va_start`.
 *               The caller is responsible for calling `va_end` afterwards.
 */
void vtrace(
    TraceCategory cat, int line, const char *file, TraceType type,
    const char *fmt, va_list vargs
);

/* -------------------------------------------------------------------------
 * Stream configuration
 * ---------------------------------------------------------------------- */

/**
 * @brief Redirects all trace output to a different stream.
 *
 * Replaces the active output stream with @p stream.  Subsequent calls to any
 * trace macro or function will write to @p stream.
 *
 * @param stream  An open, writable `FILE *`.  Passing `NULL` has undefined
 *                behaviour; use @ref DEFAULT_TRACE_STREAM to restore the
 *                default.
 *
 * @par Example
 * @code
 *  // Send all trace output to a log file.
 *  FILE *log = fopen("trace.log", "w");
 *  trace_set_stream(log);
 * @endcode
 */
void trace_set_stream(FILE *stream);

/* -------------------------------------------------------------------------
 * Runtime filter API
 * ---------------------------------------------------------------------- */

/**
 * @brief Adds a category to the category allow-list.
 *
 * After this call, messages tagged with @p cat will be allowed through the
 * category filter (subject to the type filter as well).
 *
 * @param cat  Category bit to enable, e.g. @ref TRACE_CPU.
 */
void trace_add_cat_filter(TraceCategory cat);

/**
 * @brief Removes a category from the category allow-list.
 *
 * After this call, messages tagged with @p cat will be suppressed regardless
 * of their type.
 *
 * @param cat  Category bit to disable, e.g. @ref TRACE_BUS.
 */
void trace_remove_cat_filter(TraceCategory cat);

/**
 * @brief Adds an event type to the type allow-list.
 *
 * After this call, messages of type @p type will be allowed through the type
 * filter (subject to the category filter as well).
 *
 * @param type  Type bit to enable, e.g. @ref TRACE_ERROR.
 */
void trace_add_type_filter(TraceType type);

/**
 * @brief Removes an event type from the type allow-list.
 *
 * After this call, messages of type @p type will be suppressed.
 *
 * @param type  Type bit to disable, e.g. @ref TRACE_REG_READ.
 */
void trace_remove_type_filter(TraceType type);

/**
 * @brief Resets the category filter to its default state (all categories allowed).
 *
 * Equivalent to calling @ref trace_add_cat_filter for every defined
 * @ref TraceCategory value.
 */
void trace_reset_cat_filter(void);

/**
 * @brief Resets the type filter to its default state (all types allowed).
 *
 * Equivalent to calling @ref trace_add_type_filter for every defined
 * @ref TraceType value.
 */
void trace_reset_type_filter(void);

/* -------------------------------------------------------------------------
 * Convenience macros
 * ---------------------------------------------------------------------- */

#ifdef TRACE_ON

/**
 * @def trace(type, fmt, ...)
 * @brief Emits a @ref TRACE_GENERAL trace message.
 *
 * Automatically captures the current source file and line number.
 * Compiled out entirely when `TRACE_ON` is not defined.
 *
 * @param type  @ref TraceType value, e.g. @ref TRACE_MSG or @ref TRACE_ERROR.
 * @param fmt   `printf`-style format string.
 * @param ...   Format arguments.
 */
#    define trace(...)                                                         \
        trace_(                                                                \
            TRACE_GENERAL, __LINE__, trace__get_filename(__FILE__),            \
            __VA_ARGS__                                                        \
        )

#    ifdef TRACE_BUS_ON
/**
 * @def trace_bus(type, fmt, ...)
 * @brief Emits a @ref TRACE_BUS trace message.
 *
 * Requires both `TRACE_ON` and `TRACE_BUS_ON` to be defined at compile time;
 * otherwise expands to nothing.
 *
 * @param type  @ref TraceType value.
 * @param fmt   `printf`-style format string.
 * @param ...   Format arguments.
 */
#        define trace_bus(...)                                                 \
            trace_(                                                            \
                TRACE_BUS, __LINE__, trace__get_filename(__FILE__),            \
                __VA_ARGS__                                                    \
            )
#    else
#        define trace_bus(...) /**< No-op: TRACE_BUS_ON not defined. */
#    endif

#    ifdef TRACE_CPU_ON
/**
 * @def trace_cpu(type, fmt, ...)
 * @brief Emits a @ref TRACE_CPU trace message.
 *
 * Requires both `TRACE_ON` and `TRACE_CPU_ON` to be defined at compile time;
 * otherwise expands to nothing.
 *
 * @param type  @ref TraceType value.
 * @param fmt   `printf`-style format string.
 * @param ...   Format arguments.
 */
#        define trace_cpu(...)                                                 \
            trace_(                                                            \
                TRACE_CPU, __LINE__, trace__get_filename(__FILE__),            \
                __VA_ARGS__                                                    \
            )
#    else
#        define trace_cpu(...) /**< No-op: TRACE_CPU_ON not defined. */
#    endif

#    ifdef TRACE_PPU_ON
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
#        define trace_ppu(...)                                                 \
            trace_(                                                            \
                TRACE_PPU, __LINE__, trace__get_filename(__FILE__),            \
                __VA_ARGS__                                                    \
            )
#    else
#        define trace_ppu(...) /**< No-op: TRACE_PPU_ON not defined. */
#    endif

#    ifdef TRACE_FWX_ON
/**
 * @def trace_fwx(type, fmt, ...)
 * @brief Emits a @ref TRACE_FWX trace message.
 *
 * Requires both `TRACE_ON` and `TRACE_FWX_ON` to be defined at compile time;
 * otherwise expands to nothing.  Intended for FWX protocol-level diagnostics
 * such as START/STOP conditions and data bytes.
 *
 * @param type  @ref TraceType value, typically one of @ref TRACE_START_COND,
 *              @ref TRACE_STOP_COND, @ref TRACE_TX_DATA, @ref TRACE_RX_DATA,
 *              or @ref TRACE_CLK_EDGE.
 * @param fmt   `printf`-style format string.
 * @param ...   Format arguments.
 */
#        define trace_fwx(...)                                                 \
            trace_(                                                            \
                TRACE_FWX, __LINE__, trace__get_filename(__FILE__),            \
                __VA_ARGS__                                                    \
            )
#    else
#        define trace_fwx(...) /**< No-op: TRACE_FWX_ON not defined. */
#    endif

#else  /* TRACE_ON not defined -------------------------------------------- */

/** @cond INTERNAL – all macros are no-ops when TRACE_ON is not defined. */
#    define trace(...)
#    define trace_bus(...)
#    define trace_cpu(...)
#    define trace_ppu(...)
#    define trace_fwx(...)
/** @endcond */

#endif /* TRACE_ON */

#ifdef __cplusplus
}
#endif
