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

#ifndef DEFAULT_TRACE_STREAM
#    define DEFAULT_TRACE_STREAM stdout
#endif

#ifdef __cplusplus
extern "C" {
#endif

const char *trace__get_filename(const char *path);

typedef enum TraceCategory {
    TRACE_GENERAL = 1u << 0,
    TRACE_BUS = 1u << 1,
    TRACE_CPU = 1u << 2,
    TRACE_PPU = 1u << 3,
    TRACE_FWX = 1u << 4
} TraceCategory;

typedef enum TraceType {
    TRACE_MSG = 1u << 0,
    TRACE_ERROR = 1u << 1,
    TRACE_WARN = 1u << 2,
    TRACE_REG_READ = 1u << 3,
    TRACE_REG_WRITE = 1u << 4,
    TRACE_W_LATCH = 1u << 5,
    TRACE_VBLANK = 1u << 6,
    TRACE_NMI = 1u << 7,
    TRACE_0_HIT = 1u << 8,

    /* FWX */
    TRACE_START_COND = 1u << 9,
    TRACE_STOP_COND = 1u << 10,
    TRACE_CLK_EDGE = 1u << 11,
    TRACE_TX_DATA = 1u << 12,
    TRACE_RX_DATA = 1u << 13
} TraceType;

void trace_(
    TraceCategory cat, int line, const char *file, TraceType type,
    const char *fmt, ...
);

void vtrace(
    TraceCategory cat, int line, const char *file, TraceType type,
    const char *fmt, va_list vargs
);

void trace_set_stream(FILE *stream);

void trace_add_cat_filter(TraceCategory cat);
void trace_remove_cat_filter(TraceCategory cat);
void trace_add_type_filter(TraceType type);
void trace_remove_type_filter(TraceType type);
void trace_reset_cat_filter(void);
void trace_reset_type_filter(void);

#ifdef TRACE_ON
#    define trace(...)                                                         \
        trace_(                                                                \
            TRACE_GENERAL, __LINE__, trace__get_filename(__FILE__),            \
            __VA_ARGS__                                                        \
        )

#    ifdef TRACE_BUS_ON
#        define trace_bus(...)                                                 \
            trace_(                                                            \
                TRACE_BUS, __LINE__, trace__get_filename(__FILE__),            \
                __VA_ARGS__                                                    \
            )
#    else
#        define trace_bus(...)
#    endif

#    ifdef TRACE_CPU_ON
#        define trace_cpu(...)                                                 \
            trace_(                                                            \
                TRACE_CPU, __LINE__, trace__get_filename(__FILE__),            \
                __VA_ARGS__                                                    \
            )
#    else
#        define trace_cpu(...)
#    endif

#    ifdef TRACE_PPU_ON
#        define trace_ppu(...)                                                 \
            trace_(                                                            \
                TRACE_PPU, __LINE__, trace__get_filename(__FILE__),            \
                __VA_ARGS__                                                    \
            )
#    else
#        define trace_ppu(...)
#    endif

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
