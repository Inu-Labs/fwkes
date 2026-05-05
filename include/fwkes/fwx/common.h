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

#ifndef FWX_COMMON_H
#define FWX_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/* MUST be a power of 2 */
#define FWX_MAX_DATA_SIZE 256

typedef enum FwxStatBits {
    FWX_STAT_S0 = (1 << 4),
    FWX_STAT_S1 = (1 << 5),
    FWX_STAT_S2 = (1 << 6)
} FwxStatBits;

typedef enum FwxCtrlBits {
    FWX_CTRL_SS = (1 << 0),
    FWX_CTRL_CLK = (1 << 1)
} FwxCtrlBits;

typedef enum FwxError {
    FWX_ERR_OK,
    FWX_ERR_INTERNAL,
    FWX_ERR_BAD_DATA,
    FWX_ERR_BAD_SD,
    FWX_ERR_BAD_FS,
    FWX_ERR_BAD_IDX,
    FWX_ERR_NO_EXIST,
    FWX_ERR_IS_FILE,
    FWX_ERR_IS_DIR,
} FwxError;

typedef enum FwxCmdId {
    FWX_CMD_RESET = 0x00,
    FWX_CMD_SD_INIT = 0x01,
    FWX_CMD_SD_POLL = 0x02,
    FWX_CMD_SD_LOAD = 0x03,
    FWX_CMD_SD_FILENAME = 0x04,
    FWX_CMD_SD_IS_DIR = 0x05,
    FWX_CMD_SD_PWD = 0x06,
    FWX_CMD_SD_CWD = 0x07,
    FWX_CMD_SD_FILE_COUNT = 0x08,
    FWX_CMD_LED_ERROR = 0x09,
    FWX_CMD_LED_GENERAL = 0x0a,
    FWX_CMD_SD_GET_ENTRY = 0x0b
} FwxCmdId;

#ifdef __cplusplus
}
#endif

#endif /* FWX_COMMON_H */
