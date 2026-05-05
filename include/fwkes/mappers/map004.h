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

#include "shared.h"

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Mapper4 {
    uint8_t bank_select;
    uint8_t bank_regs[8];
    uint8_t prg_mode;
    uint8_t chr_mode;
    uint8_t prg_bank_count;
    uint8_t *prg_banks[4];
    uint8_t *chr_banks[8];
    uint8_t irq_cnt;
    bool irq_enabled;
    bool irq_reload_pending;
    uint8_t irq_latch;
} Mapper4;

DECLARE_MAPPER_FULL_INTERFACE(4);
void mapper4_update_prg(Disk *self);
void mapper4_update_chr(Disk *self);

#ifdef __cplusplus
}
#endif
