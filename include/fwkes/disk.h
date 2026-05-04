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

#include "fs.h"
#include "mappers/map000.h"
#include "mappers/map002.h"
#include "mappers/map004.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define PRG_ROM_UNIT 16384
#define CHR_ROM_UNIT 8192
#define PRG_RAM_SIZE 8192

#define INES_SIZE 0x10

#define INES_FLAGS6_NAMETABLE_MIRRORING (1u << 0)
#define INES_FLAGS6_BATTERY (1u << 1)
#define INES_FLAGS6_TRAINER (1u << 2)
#define INES_FLAGS6_ALTERNATIVE_NAMETABLE (1u << 3)
#define INES_FLAGS6_MAPPER_LN 0xf0

#define INES_FLAGS7_VS (1u << 0)
#define INES_FLAGS7_PLAYCHOICE (1u << 1)
#define INES_FLAGS7_NES2 0xc
#define INES_FLAGS7_MAPPER_HN 0xf0

#define INES_FLAGS9_TV (1u << 0)

#define INES_FLAGS10_TV 0x3
#define INES_FLAGS10_PRG_RAM (1u << 4)
#define INES_FLAGS10_BUS_CONFLICTS (1u << 5)

#ifdef __cplusplus
extern "C" {
#endif

typedef enum Mirroring {
    MIRRORING_VERTICAL,
    MIRRORING_HORIZONTAL,
} Mirroring;

typedef struct DiskINes {
    uint8_t prg_rom_size; /* units of 16 KiB */
    uint8_t chr_rom_size; /* units of 8 KiB */
    uint8_t flags6;
    uint8_t flags7;
    uint8_t flags8;
    uint8_t flags9;
    uint8_t flags10;
} DiskINes;

typedef enum DiskFormat {
    DISK_ERR,
    DISK_INES,
} DiskFormat;

bool disk_ines_parse(DiskINes *self, const uint8_t *rom);

typedef struct Bus Bus;

typedef struct Disk {
    Bus *bus;

    DiskFormat fmt;

    union {
        DiskINes ines;
    };

    MapperId mapper;

    union {
        Mapper0 map0;
        Mapper2 map2;
        Mapper4 map4;
    };

    uint8_t *rom;
    unsigned size;
    uint8_t *prg;
    unsigned prg_size;
    uint8_t *prg_ram;
    uint8_t *chr;
    unsigned chr_size;
    bool chr_is_ram;
    Mirroring mirroring;

    MapperWriteFn mapper_write;
    MapperReadFn mapper_read;
    MapperPpuReadFn mapper_ppu_read;
    MapperPpuWriteFn mapper_ppu_write;
    MapperHsyncFn mapper_hsync;
} Disk;

bool disk_load(Disk *self, Bus *bus, Fs *fs, const char *path);
bool disk_load_mem(Disk *self, Bus *bus, const uint8_t *data, unsigned size);
void disk_unload(Disk *self);

#ifdef __cplusplus
}
#endif
