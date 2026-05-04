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

#include <stdarg.h>
#include <stdio.h>

#ifndef DEFAULT_LOG_STREAM
#    define DEFAULT_LOG_STREAM stdout
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum LogCategory {
    LOG_GENERAL = (1u << 0),
    LOG_BUS = (1u << 1),
    LOG_CPU = (1u << 2),
    LOG_PPU = (1u << 3)
} LogCategory;

typedef enum LogType {
    LOG_INFO = (1u << 0),
    LOG_ERROR = (1u << 1),
    LOG_WARN = (1u << 2),
    LOG_READ = (1u << 3),
    LOG_WRITE = (1u << 4),
    LOG_RW = (1u << 5),
} LogType;

void log_msg_(LogCategory cat, LogType type, const char *fmt, ...);
void vlog_msg_(LogCategory cat, LogType type, const char *fmt, va_list vargs);

void log_set_stream(FILE *stream);

void log_add_cat_filter(LogCategory cat);
void log_remove_cat_filter(LogCategory cat);
void log_add_type_filter(LogType type);
void log_remove_type_filter(LogType type);
void log_reset_cat_filter(void);
void log_reset_type_filter(void);

#ifndef BUILD_RP2350
#define log_msg(...) log_msg_(LOG_GENERAL, __VA_ARGS__)
#define log_bus(...) log_msg_(LOG_BUS, __VA_ARGS__)
#define log_cpu(...) log_msg_(LOG_CPU, __VA_ARGS__)
#define log_ppu(...) log_msg_(LOG_PPU, __VA_ARGS__)
#else
#define log_msg(...)
#define log_bus(...)
#define log_cpu(...)
#define log_ppu(...)
#endif

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
