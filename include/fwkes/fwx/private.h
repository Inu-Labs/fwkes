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
 * @file fwx/private.h
 * @brief Emulator-side implementation of FWX protocol.
 */

#include "../disk.h"
#include "../fs.h"
#include "common.h"

#include <stdint.h>

#define FWX_STAT 0x4018
#define FWX_CTRL 0x4019
#define FWX_DATA 0x401a

/**
 * Exposed registers to emulated program.
 */
typedef struct FwxRegs {
    uint8_t STAT;
    uint8_t CTRL;
    uint8_t DATA;
} FwxRegs;

typedef char FileItem[256];

/**
 * Internal data buffer.
 */
typedef struct FwxBuffer {
    uint8_t data[FWX_MAX_DATA_SIZE];
    unsigned size;
    unsigned expected_size;
    unsigned pos;
} FwxBuffer;

/**
 * @brief Set buffer cursor to 0.
 * @param self Buffer instance.
 */
static inline void fwx_buf_rewind(FwxBuffer *self) { self->pos = 0; }

/**
 * @brief Reset buffer state: data are zeroed, size and pos are set to 0, expected_size is set to
 * \ref FWX_MAX_DATA_SIZE.
 * @param self Buffer instance.
 */
void fwx_buf_reset(FwxBuffer *self);
/**
 * @brief Append a byte at the current cursor position (pos). Cursor position is moved forward
 * (incremented).
 * @param self Buffer instance.
 * @param data Byte to put.
 */
void fwx_buf_put(FwxBuffer *self, uint8_t data);
/**
 * @brief Get byte at the current cursor position (pos). Cursor position is moved forward
 * (incremented).
 * @param self Buffer instance.
 * @returns Byte pointed by pos (before it is incremented).
 */
uint8_t fwx_buf_get(FwxBuffer *self);

/**
 * @brief Get pointer to byte at the current cursor position (pos).
 * @param self Buffer instance.
 * @returns Pointer to the byte pointed by pos.
 */
static inline uint8_t *fwx_buf_ref(FwxBuffer *self) {
    return &self->data[self->pos];
}

typedef struct Bus Bus;

/**
 * State of FWX component.
 */
typedef struct Fwx {
    FwxRegs regs;
    FwxBuffer tx; /**> Buffer for data transmission from emulator to the emulated program. */
    FwxBuffer rx; /**> Buffer for data transmission from the emulated program to emulator. */

    Fs *fs;
    Bus *bus;
    Dir cwd;
    Disk disk;

    FwxCmdId curr_cmd; /**> Current command ID (first byte from FWXDATA). */
    unsigned file_count /**> Number of files in the current working directory. */;

    /* TODO: consider memory usage optimization (61.44 kB!) */
    char files[256][256]; /**> List of filenames in the current working directory. */
} Fwx;

/**
 * @brief Get pointer to byte at the current cursor position (pos).
 * @param self Fwx instance.
 * @param fs Pointer to @ref Fs instance.
 * @param bus Pointer to @ref Bus instance.
 * @retval true Success.
 * @retval false Failure.
 */
bool fwx_init(Fwx *self, Fs *fs, Bus *bus);
/**
 * @brief Reset FWX state.
 *
 * Buffers, registers, current error and command are zeroed. File list is updated.
 *
 * @param self Fwx instance.
 * @retval true Success.
 * @retval false Failure when trying to reopen current working directory.
 */
bool fwx_reset(Fwx *self);
/**
 * @brief Reset state and free used resources.
 * @param self Fwx instance.
 */
void fwx_deinit(Fwx *self);

/**
 * @brief Get value of FWXDATA.
 * @param self @ref Fwx instance.
 * @returns Value of FWXDATA.
 */
static inline uint8_t fwx_read_data(const Fwx *self) { return self->regs.DATA; }

/**
 * @brief Set value of FWXDATA.
 * @param self @ref Fwx instance.
 * @param data Byte to write.
 */
static inline void fwx_write_data(Fwx *self, uint8_t data) {
    self->regs.DATA = data;
}

/**
 * @brief Update file list of the current working directory.
 * @param self @ref Fwx instance.
 * @returns Error code of last successful (or onsuccessful) filesystem operation.
 */
FsError fwx_update_file_list(Fwx *self);
/**
 * @brief Set last error code.
 * @param self @ref Fwx instance.
 * @param err Error code to set.
 */
void fwx_set_error(Fwx *self, FwxError err);
/**
 * @brief MMIO write interface for bus.
 * @param self @ref Fwx instance.
 * @param addr Register address.
 * @param data Byte to write.
 */
void fwx_write(Fwx *self, uint16_t addr, uint8_t data);
/**
 * @brief MMIO read interface (with side effects) for bus.
 *
 * When reading FWXSTAT, the S2 bit is cleared.
 *
 * @param self @ref Fwx instance.
 * @param addr Register address.
 * @returns Value stored or returned by accessed register.
 */
uint8_t fwx_read(Fwx *self, uint16_t addr);
/**
 * @brief MMIO read interface without side effects for bus.
 * @param self @ref Fwx instance.
 * @param addr Register address.
 * @returns Value stored or returned by accessed register.
 */
uint8_t fwx_peek(const Fwx *self, uint16_t addr);
/**
 * @brief Read FWXSTAT register with side effects.
 *
 * This register reports the status of communication.
 *
 * When reading FWXSTAT, the S2 bit is cleared.
 *
 * @param self @ref Fwx instance.
 * @returns Value stored or returned by accessed register.
 */
uint8_t fwx_read_stat(Fwx *self);
/**
 * @brief Write to FWXCTRL register with proper handling.
 *
 * This function controls communication, and its behavior depends on bits in @p value:
 *
 * - New SS = 1, old SS = 0: START condition.
 * - New SS = 0, old SS = 1: STOP condition
 * - SS = 1, new CLK = 1, old CLK = 0: edge clock
 *
 * @par START condition
 *
 * START condition begins communication. TX & RX buffers, current error and command are zeroed,
 * except that S0 in FWXSTAT is set to 1.
 *
 * @par STOP condition
 *
 * STOP condition terminates communication. Current error and S0..S2 in FWXSTAT are zeroed, although
 * buffers are not touched.
 *
 * @par Clock edge
 *
 * Clock edge triggers data transmission from emulator to emulated program (S1 is 1), or from
 * emulated program to emulator (S1 is 0). Data transmission happens in a bytewise fashion.
 *
 * @param self @ref Fwx instance.
 * @param value Value to write.
 */
void fwx_write_ctrl(Fwx *self, uint8_t value);
