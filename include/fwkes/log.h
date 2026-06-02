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
 * @file log.h
 * @brief Logging subsystem.
 *
 * Lightweight loging facility designed for
 * use in both embedded (RP2350) and desktop host builds.
 *
 * Each log call has assigned category, that maps to logical subsystem, e.g.
 * CPU, PPU, bus, etc.; and a **type** (error, warning, ...). It's also possible
 * to filter (ignore) messages based on category or type.
 *
 * @par Debug logging
 *
 * When `NESTEST` is enabled, there are also debug variants of log functions,
 * that start with prefix `dlog_`. Those variants are intended for developers,
 * not for end-users.
 *
 * @par Runtime filtering
 *
 *  Category and type filters are bitmasks maintained internally.  By default
 *  all categories and types are allowed through.  Use
 *  @ref log_add_cat_filter / @ref log_remove_cat_filter and
 *  @ref log_add_type_filter / @ref log_remove_type_filter to narrow the
 *  output at runtime without recompiling.
 *
 * @par Output stream
 *
 * The output destination by default goes to `stdout`, but it can be set to
 * different output stream using log_set_stream(). Alternatively, one can
 * redefine macro `DEFAULT_LOG_STREAM`, although it requires recompiling log
 * source files.
 */

#pragma once

#include <stdarg.h>
#include <stdio.h>
/**
 * @def DEFAULT_LOG_STREAM
 * @brief Default `FILE *` stream used for logging output.
 */
#ifndef DEFAULT_LOG_STREAM
#    define DEFAULT_LOG_STREAM stdout
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Logical subsystem categories for logging messages.
 *
 * Can be combined in bitmasks to assign multiple categories for a message.
 */
typedef enum LogCategory {
    LOG_GENERAL = (1u << 0),
    LOG_BUS = (1u << 1),
    LOG_CPU = (1u << 2),
    LOG_PPU = (1u << 3)
} LogCategory;

/**
 * @brief Event-type tags for logging messages.
 *
 * Can be combined in bitmasks to assign multiple tags for a message.
 */
typedef enum LogType {
    LOG_INFO = (1u << 0),
    LOG_ERROR = (1u << 1),
    LOG_WARN = (1u << 2),
    LOG_READ = (1u << 3),
    LOG_WRITE = (1u << 4),
    LOG_RW = (1u << 5),
} LogType;

/**
 * @brief Emits a formatted log  message (internal implementation).
 *
 * This is the underlying implementation called by all category-specific
 * macros.  Prefer the macros (@ref log_msg, @ref log_bus, ...) over calling
 * this function directly.
 *
 * The message is only written to the output stream if both @p cat and
 * @p type pass the current runtime filters (see @ref log_add_cat_filter and
 * @ref log_add_type_filter).
 *
 * @param cat   Subsystem category @ref LogCategory.
 * @param type  Event type @ref LogType.
 * @param fmt   `printf`-style format string.
 * @param ...   Format arguments matching @p fmt.
 */
void log_msg_(LogCategory cat, LogType type, const char *fmt, ...);

/**
 * @brief `va_list` variant of @ref log_.
 *
 * Identical to @ref log_ but accepts a `va_list` instead of variadic
 * arguments.
 *
 * @param cat   Subsystem category @ref LogCategory.
 * @param type  Event type @ref LogType.
 * @param fmt   `printf`-style format string.
 * @param ...   Format arguments matching @p fmt.
 * @param vargs  Argument list initialized by the caller with `va_start`.
 */
void vlog_msg_(LogCategory cat, LogType type, const char *fmt, va_list vargs);

/**
 * @brief Set output stream for all log messages.
 *
 * @param stream  An open, writable `FILE *`.  Passing `NULL` has undefined
 *                behaviour; use @ref DEFAULT_LOG_STREAM to restore the
 *                default.
 */
void log_set_stream(FILE *stream);

/**
 * @brief Add a category to the category blacklist.
 *
 * After this call, messages tagged with @p cat will be suppressed.
 *
 * @param cat  Category bit to enable.
 */
void log_add_cat_filter(LogCategory cat);

/**
 * @brief Remove a category from the category blacklist.
 *
 * After this call, messages tagged with @p cat will be allowed.
 *
 * @param cat  Category bit to disable.
 */
void log_remove_cat_filter(LogCategory cat);

/**
 * @brief Add a event-type tag to the category blacklist.
 *
 * After this call, messages tagged with @p type will be suppressed.
 *
 * @param type  Tag bit to enable.
 */
void log_add_type_filter(LogType type);

/**
 * @brief Remove a event-type tag to the category blacklist.
 *
 * After this call, messages tagged with @p type will be allowed.
 *
 * @param type  Tag bit to enable.
 */
void log_remove_type_filter(LogType type);

/**
 * @brief Resets the category filter to its default state (all categories
 * allowed).
 */
void log_reset_cat_filter(void);

/**
 * @brief Resets the type filter to its default state (all types allowed).
 */
void log_reset_type_filter(void);

/**
 * @def log_msg(type, fmt, ...)
 * @brief Emits a @ref LOG_GENERAL log message.
 *
 * @param type  @ref LogType value.
 * @param fmt   `printf`-style format string.
 */

/**
 * @def log_bus(type, fmt, ...)
 * @brief Emits a @ref LOG_BUS log message.
 *
 * @param type  @ref LogType value.
 * @param fmt   `printf`-style format string.
 */

/**
 * @def log_cpu(type, fmt, ...)
 * @brief Emits a @ref LOG_CPU log message.
 *
 * @param type  @ref LogType value.
 * @param fmt   `printf`-style format string.
 */

/**
 * @def log_ppu(type, fmt, ...)
 * @brief Emits a @ref LOG_PPU log message.
 *
 * @param type  @ref LogType value.
 * @param fmt   `printf`-style format string.
 */
#ifndef BUILD_RP2350
#    define log_msg(...) log_msg_(LOG_GENERAL, __VA_ARGS__)
#    define log_bus(...) log_msg_(LOG_BUS, __VA_ARGS__)
#    define log_cpu(...) log_msg_(LOG_CPU, __VA_ARGS__)
#    define log_ppu(...) log_msg_(LOG_PPU, __VA_ARGS__)
#else
#    define log_msg(...)
#    define log_bus(...)
#    define log_cpu(...)
#    define log_ppu(...)
#endif

/**
 * @def dlog_msg(type, fmt, ...)
 * @brief Debug variant of @ref log_msg().
 *
 * `NESTEST` must be defined, otherwise expands to nothing.
 *
 * @param type  @ref LogType value.
 * @param fmt   `printf`-style format string.
 */

/**
 * @def dlog_bus(type, fmt, ...)
 * @brief Debug variant of @ref log_bus().
 *
 * `NESTEST` must be defined, otherwise expands to nothing.
 *
 * @param type  @ref LogType value.
 * @param fmt   `printf`-style format string.
 */

/**
 * @def dlog_cpu(type, fmt, ...)
 * @brief Debug variant of @ref log_cpu().
 *
 * `NESTEST` must be defined, otherwise expands to nothing.
 *
 * @param type  @ref LogType value.
 * @param fmt   `printf`-style format string.
 */

/**
 * @def dlog_ppu(type, fmt, ...)
 * @brief Debug variant of @ref log_ppu().
 *
 * `NESTEST` must be defined, otherwise expands to nothing.
 *
 * @param type  @ref LogType value.
 * @param fmt   `printf`-style format string.
 */
#ifdef NESTEST
#    define dlog_msg(...) log_msg_(LOG_GENERAL, __VA_ARGS__)
#    define dlog_bus(...) log_msg_(LOG_BUS, __VA_ARGS__)
#    define dlog_cpu(...) log_msg_(LOG_CPU, __VA_ARGS__)
#    define dlog_ppu(...) log_msg_(LOG_PPU, __VA_ARGS__)
#else
#    define dlog_msg(...)
#    define dlog_bus(...)
#    define dlog_cpu(...)
#    define dlog_ppu(...)
#endif

#ifdef __cplusplus
}
#endif
