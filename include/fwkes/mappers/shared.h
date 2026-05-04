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

#include "../util.h"

#include <stddef.h>
#include <stdint.h>

#define DECLARE_MAPPER_CPU_INTERFACE(id)                                       \
    void NOTFLASH_FN(mapper##id##_write)(                                      \
        Disk * disk, uint16_t addr, uint8_t data                               \
    );                                                                         \
    uint8_t mapper##id##_read(Disk *disk, uint16_t addr)

#define DECLARE_MAPPER_PPU_INTERFACE(id)                                       \
    void NOTFLASH_FN(mapper##id##_ppu_write)(                                  \
        Disk * disk, uint16_t addr, uint8_t data                               \
    );                                                                         \
    uint8_t NOTFLASH_FN(mapper##id##_ppu_read)(Disk * disk, uint16_t addr)

#define DECLARE_MAPPER_HSYNC_FN(id)                                            \
    void NOTFLASH_FN(mapper##id##_hsync)(Disk * disk)

#define DECLARE_MAPPER_FULL_INTERFACE(id)                                      \
    DECLARE_MAPPER_CPU_INTERFACE(id);                                          \
    DECLARE_MAPPER_PPU_INTERFACE(id);                                          \
    DECLARE_MAPPER_HSYNC_FN(id)

typedef struct Disk Disk;
typedef unsigned MapperId;

typedef void (*MapperWriteFn)(Disk *disk, uint16_t addr, uint8_t data);
typedef uint8_t (*MapperReadFn)(Disk *disk, uint16_t addr);
typedef void (*MapperPpuWriteFn)(Disk *disk, uint16_t addr, uint8_t data);
typedef uint8_t (*MapperPpuReadFn)(Disk *disk, uint16_t addr);
/* Called at the end of each visible scaline when rendering is ON */
typedef void (*MapperHsyncFn)(Disk *disk);
