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

#include <fwkes/trace.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if __unix__
#    include <unistd.h>
#elif _WIN32
#    include <io.h>
#endif

static TraceCategory g_cat_filter = 0;
static TraceType g_type_filter = 0;
static FILE *g_stream = NULL;

static const char *g_cat_str[] = {
    "",       /* TRACE_GENERAL */
    "[BUS] ", /* TRACE_BUS */
    "[CPU] ", /* TRACE_CPU */
    "[PPU] ", /* TRACE_PPU */
    "[FWX] "  /* TRACE_FWX */
};

static const char *g_cat_color_mod[] = {
    "",         /* TRACE_GENERAL */
    "\033[33m", /* TRACE_BUS */
    "\033[36m", /* TRACE_CPU */
    "\033[31m", /* TRACE_PPU */
    "\033[31m"  /* TRACE_FWX */
};

static const char *g_type_color_mod[] = {
    "",                /* TRACE_MSG */
    "\033[31m\033[1m", /* TRACE_ERROR */
    "\033[33m\033[1m", /* TRACE_WARN */
    "",                /* TRACE_REG_READ */
    "",                /* TRACE_REG_WRITE */
    "",                /* TRACE_W_LATCH */
    "",                /* TRACE_VBLANK */
    "",                /* TRACE_NMI */
    "",                /* TRACE_0_HIT */
    "",                /* TRACE_START_COND */
    "",                /* TRACE_STOP_COND */
    "",                /* TRACE_CLK_EDGE */
    "",                /* TRACE_TX_DATA */
    "",                /* TRACE_RX_DATA */
};

static const char *g_type_str[] = {"INFO",      "ERR",     "WARN",   "REG_READ",
                                   "REG_WRITE", "W_LATCH", "VBLANK", "NMI",
                                   "SPR_0_HIT", "START",   "STOP",   "CLK",
                                   "TX DATA",   "RX DATA"};

const char *trace__get_filename(const char *path) {
    const char *filename = strrchr(path, '/');

    if (filename) {
        return filename + 1;
    }

    filename = strrchr(path, '\\');

    if (filename) {
        return filename + 1;
    }

    return path;
}

static unsigned cat_to_idx(TraceCategory cat) {
    switch (cat) {
    case TRACE_GENERAL:
        return 0;
    case TRACE_BUS:
        return 1;
    case TRACE_CPU:
        return 2;
    case TRACE_PPU:
        return 3;
    case TRACE_FWX:
        return 4;
    }
}

static unsigned type_to_idx(TraceType cat) {
    switch (cat) {
    case TRACE_MSG:
        return 0;
    case TRACE_ERROR:
        return 1;
    case TRACE_WARN:
        return 2;
    case TRACE_REG_READ:
        return 3;
    case TRACE_REG_WRITE:
        return 4;
    case TRACE_W_LATCH:
        return 5;
    case TRACE_VBLANK:
        return 6;
    case TRACE_NMI:
        return 7;
    case TRACE_0_HIT:
        return 8;
    case TRACE_START_COND:
        return 9;
    case TRACE_STOP_COND:
        return 10;
    case TRACE_CLK_EDGE:
        return 11;
    case TRACE_TX_DATA:
        return 12;
    case TRACE_RX_DATA:
        return 13;
    }
}

void trace_(
    TraceCategory cat, int line, const char *filename, TraceType type,
    const char *fmt, ...
) {
    va_list vargs;
    va_start(vargs, fmt);
    vtrace(cat, line, filename, type, fmt, vargs);
    va_end(vargs);
}

void vtrace(
    TraceCategory cat, int line, const char *filename, TraceType type,
    const char *fmt, va_list vargs
) {
    if ((g_cat_filter & cat) || (g_type_filter & type)) {
        return;
    }

    if (!g_stream) {
        g_stream = stdout;
    }

    unsigned cat_idx = cat_to_idx(cat);
    unsigned type_idx = type_to_idx(type);
    const char *cat_str = g_cat_str[cat_idx];
    const char *type_str = g_type_str[type_idx];

#if _WIN32
    if (_isatty(_fileno(g_stream))) {
#elif __unix__
    if (isatty(fileno(g_stream))) {
#else
    if (0) {
#endif
        fprintf(
            g_stream, "\033[2m%s%sTRACE %s%s:%d %s: ", g_cat_color_mod[cat_idx],
            g_type_color_mod[type_idx], cat_str, filename, line, type_str
        );
        vfprintf(g_stream, fmt, vargs);
        fputs("\033[0m\n", g_stream);
    } else {
        fprintf(
            g_stream, "TRACE [%s] %s:%d %s: ", cat_str, filename, line, type_str
        );
        vfprintf(g_stream, fmt, vargs);
        fputc('\n', g_stream);
    }
}

void trace_set_stream(FILE *stream) { g_stream = stream; }

void trace_add_cat_filter(TraceCategory cat) { g_cat_filter |= cat; }

void trace_remove_cat_filter(TraceCategory cat) { g_cat_filter &= ~cat; }

void trace_add_type_filter(TraceType type) { g_type_filter |= type; }

void trace_remove_type_filter(TraceType type) { g_type_filter &= ~type; }

void trace_reset_cat_filter(void) { g_cat_filter = 0; }

void trace_reset_type_filter(void) { g_type_filter = 0; }
