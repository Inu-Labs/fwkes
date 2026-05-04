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

#include "../disk.h"
#include "../fs.h"
#include "common.h"

#include <stdint.h>

#define FWX_STAT 0x4018
#define FWX_CTRL 0x4019
#define FWX_DATA 0x401a

typedef struct FwxRegs {
    volatile uint8_t STAT;
    volatile uint8_t CTRL;
    volatile uint8_t DATA;
} FwxRegs;

typedef char FileItem[256];

typedef struct FwxBuffer {
    uint8_t data[FWX_MAX_DATA_SIZE];
    unsigned size;
    unsigned expected_size;
    unsigned pos;
} FwxBuffer;

static inline void fwx_buf_rewind(FwxBuffer *self) { self->pos = 0; }

void fwx_buf_reset(FwxBuffer *self);
void fwx_buf_put(FwxBuffer *self, uint8_t data);
uint8_t fwx_buf_get(FwxBuffer *self);

static inline uint8_t *fwx_buf_ref(FwxBuffer *self) {
    return &self->data[self->pos];
}

typedef struct Bus Bus;

typedef struct Fwx {
    FwxRegs regs;
    FwxBuffer tx; /* emu -> host */
    FwxBuffer rx; /* host -> emu */

    Fs *fs;
    Bus *bus;
    Dir cwd;
    Disk disk;

    FwxCmdId curr_cmd;
    unsigned file_count;

    /* TODO: consider memory usage optimization (61.44 kB!) */
    char files[256][256];
} Fwx;

bool fwx_init(Fwx *self, Fs *fs, Bus *bus);
bool fwx_reset(Fwx *self);
void fwx_deinit(Fwx *self);

static inline uint8_t fwx_read_data(const Fwx *self) { return self->regs.DATA; }

static inline void fwx_write_data(Fwx *self, uint8_t data) {
    self->regs.DATA = data;
}

FsError fwx_update_file_list(Fwx *self);
void fwx_set_error(Fwx *self, FwxError err);
void fwx_write(Fwx *self, uint16_t addr, uint8_t data);
uint8_t fwx_read(Fwx *self, uint16_t addr);
uint8_t fwx_peek(const Fwx *self, uint16_t addr);
uint8_t fwx_read_stat(Fwx *self);
void fwx_write_ctrl(Fwx *self, uint8_t value);
