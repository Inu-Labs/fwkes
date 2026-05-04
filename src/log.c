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

#include <fwkes/log.h>

#include <stdarg.h>
#include <stdio.h>

#if __unix__
#    include <unistd.h>
#elif _WIN32
#    include <io.h>
#endif

static LogCategory g_cat_filter = 0;
static LogType g_type_filter = 0;
static FILE *g_stream = NULL;

static const char *g_cat_str[] = {"", "[BUS] ", "[CPU] ", "[PPU] "};

static const char *g_cat_color_mod[] = {"", "\033[33m", "\033[36m", "\033[31m"};

static const char *g_type_color_mod[] = {
    "", "\033[31m\033[1m", "\033[33m\033[1m", "", "", ""
};

static const char *g_type_str[] = {"INFO", "ERR", "WARN", "R", "W", "RW"};

static unsigned cat_to_idx(LogCategory cat) {
    switch (cat) {
    case LOG_GENERAL:
        return 0;
    case LOG_BUS:
        return 1;
    case LOG_CPU:
        return 2;
    case LOG_PPU:
        return 3;
    }
}

static unsigned type_to_idx(LogType cat) {
    switch (cat) {
    case LOG_INFO:
        return 0;
    case LOG_ERROR:
        return 1;
    case LOG_WARN:
        return 2;
    case LOG_READ:
        return 3;
    case LOG_WRITE:
        return 4;
    case LOG_RW:
        return 5;
    }
}

void log_msg_(LogCategory cat, LogType type, const char *fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);
    vlog_msg_(cat, type, fmt, vargs);
    va_end(vargs);
}

void vlog_msg_(LogCategory cat, LogType type, const char *fmt, va_list vargs) {
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
    if(0) {
#endif
        fputs(g_cat_color_mod[cat_idx], g_stream);
        fputs(g_type_color_mod[type_idx], g_stream);
        fputs(cat_str, g_stream);
        fputs(type_str, g_stream);
        fputs(": ", g_stream);
        vfprintf(g_stream, fmt, vargs);
        fputs("\033[0m\n", g_stream);
    } else {
        fputs(cat_str, g_stream);
        fputs(type_str, g_stream);
        fputs(": ", g_stream);
        vfprintf(g_stream, fmt, vargs);
        fputc('\n', g_stream);
    }
}

void log_set_stream(FILE *stream) { g_stream = stream; }

void log_add_cat_filter(LogCategory cat) { g_cat_filter |= cat; }

void log_remove_cat_filter(LogCategory cat) { g_cat_filter &= ~cat; }

void log_add_type_filter(LogType type) { g_type_filter |= type; }

void log_remove_type_filter(LogType type) { g_type_filter &= ~type; }

void log_reset_cat_filter(void) { g_cat_filter = 0; }

void log_reset_type_filter(void) { g_type_filter = 0; }
