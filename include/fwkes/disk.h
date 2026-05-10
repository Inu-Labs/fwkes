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

/**
 * @file disk.h
 * @brief NES cartridge (ROM) loading and mapper management.
 *
 * In NES terminology a "disk" or "cartridge" is the game ROM file together
 * with the hardware that was inside the physical cartridge – most importantly
 * the *mapper*, a small circuit that extended the NES's limited address space
 * so larger or more complex games could fit.
 *
 * This file handles:
 *   - Parsing the iNES file format (the standard way ROMs are distributed).
 *   - Splitting the ROM data into PRG (program code) and CHR (graphics) regions.
 *   - Selecting and initialising the correct mapper for the loaded game.
 *   - Providing the CPU and PPU read/write hooks the mapper exposes.
 *
 * ## iNES format primer
 * An iNES file starts with a 16-byte header followed by the PRG ROM data and
 * then the CHR ROM data.  The header encodes how big each region is and which
 * mapper chip the cartridge uses.  Almost all ROM files you will find online
 * use this format.
 */

#pragma once

#include "fs.h"
#include "mappers/map000.h"
#include "mappers/map002.h"
#include "mappers/map004.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* =========================================================================
 * ROM region size constants
 * ========================================================================= */

/** Size of one PRG ROM bank in bytes (16 KiB).
 *  The iNES header stores the PRG ROM size as a count of these units. */
#define PRG_ROM_UNIT 16384

/** Size of one CHR ROM bank in bytes (8 KiB).
 *  The iNES header stores the CHR ROM size as a count of these units. */
#define CHR_ROM_UNIT 8192

/** Size of the PRG RAM region in bytes (8 KiB).
 *  PRG RAM is battery-backed save RAM on cartridges that support it. */
#define PRG_RAM_SIZE 8192

/** Size of the iNES file header in bytes. */
#define INES_SIZE 0x10

/* =========================================================================
 * iNES header – Flags 6
 *
 * The first flags byte carries basic mirroring, battery, and mapper info.
 * ========================================================================= */

/** @defgroup ines_flags6 iNES Flags 6
 *  @{
 */

/** Bit 0: Nametable mirroring direction.
 *  0 = horizontal mirroring (vertical scroll games),
 *  1 = vertical mirroring (horizontal scroll games). */
#define INES_FLAGS6_NAMETABLE_MIRRORING (1u << 0)

/** Bit 1: Battery-backed PRG RAM present.
 *  When set the cartridge has a coin-cell battery keeping save data alive. */
#define INES_FLAGS6_BATTERY (1u << 1)

/** Bit 2: 512-byte trainer present.
 *  A trainer is a small patch block inserted before the PRG ROM by certain
 *  copiers.  Almost no legitimate ROM file uses this. */
#define INES_FLAGS6_TRAINER (1u << 2)

/** Bit 3: Alternative nametable layout (four-screen VRAM).
 *  When set the cartridge provides its own extra VRAM for all four nametables,
 *  overriding the normal mirroring setting. */
#define INES_FLAGS6_ALTERNATIVE_NAMETABLE (1u << 3)

/** Bits 4-7: Lower nibble of the mapper number. */
#define INES_FLAGS6_MAPPER_LN 0xf0

/** @} */

/* =========================================================================
 * iNES header – Flags 7
 *
 * The second flags byte carries console type and the upper mapper nibble.
 * ========================================================================= */

/** @defgroup ines_flags7 iNES Flags 7
 *  @{
 */

/** Bit 0: VS. Unisystem ROM (arcade cabinet variant of the NES). */
#define INES_FLAGS7_VS (1u << 0)

/** Bit 1: PlayChoice-10 ROM (arcade cabinet with a small hint screen). */
#define INES_FLAGS7_PLAYCHOICE (1u << 1)

/** Bits 2-3: iNES 2.0 magic value.
 *  If these two bits equal 0b10 the file uses the extended iNES 2.0 format. */
#define INES_FLAGS7_NES2 0xc

/** Bits 4-7: Upper nibble of the mapper number.
 *  Combined with INES_FLAGS6_MAPPER_LN to get the full 8-bit mapper ID. */
#define INES_FLAGS7_MAPPER_HN 0xf0

/** @} */

/* =========================================================================
 * iNES header – Flags 9
 * ========================================================================= */

/** @defgroup ines_flags9 iNES Flags 9
 *  @{
 */

/** Bit 0: TV system.  0 = NTSC (North America / Japan), 1 = PAL (Europe). */
#define INES_FLAGS9_TV (1u << 0)

/** @} */

/* =========================================================================
 * iNES header – Flags 10  (unofficial / rarely used)
 * ========================================================================= */

/** @defgroup ines_flags10 iNES Flags 10
 *  @{
 */

/** Bits 0-1: TV system (more detailed than flags9). */
#define INES_FLAGS10_TV 0x3

/** Bit 4: PRG RAM present flag (unofficial extension). */
#define INES_FLAGS10_PRG_RAM (1u << 4)

/** Bit 5: Bus conflicts present.
 *  Some cartridges have a hardware quirk where writing to ROM causes the
 *  written value to be AND-ed with the value already in ROM.  Mappers that
 *  need this behaviour check this flag. */
#define INES_FLAGS10_BUS_CONFLICTS (1u << 5)

/** @} */

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Types
 * ========================================================================= */

/**
 * @brief Nametable mirroring mode.
 *
 * Controls how the PPU's two physical nametable RAM banks are mapped to the
 * four logical nametable addresses ($2000, $2400, $2800, $2C00).
 *
 * - **Vertical** mirroring: $2000 = $2800, $2400 = $2C00.
 *   Used by horizontally-scrolling games (e.g. Super Mario Bros.).
 * - **Horizontal** mirroring: $2000 = $2400, $2800 = $2C00.
 *   Used by vertically-scrolling games (e.g. Donkey Kong).
 */
typedef enum Mirroring {
    MIRRORING_VERTICAL,   /**< Left and right nametables are mirrored (horizontal scroll). */
    MIRRORING_HORIZONTAL, /**< Top and bottom nametables are mirrored (vertical scroll). */
} Mirroring;

/**
 * @brief Parsed contents of an iNES file header.
 *
 * This is filled in by @ref disk_ines_parse from the raw 16-byte header at
 * the start of the ROM file.  All size fields use the units defined above
 * (PRG_ROM_UNIT, CHR_ROM_UNIT).
 */
typedef struct DiskINes {
    uint8_t prg_rom_size; /**< PRG ROM size in 16 KiB units (multiply by PRG_ROM_UNIT for bytes). */
    uint8_t chr_rom_size; /**< CHR ROM size in 8 KiB units (0 means the game uses CHR RAM instead). */
    uint8_t flags6;       /**< Flags byte 6: mirroring, battery, trainer, lower mapper nibble. */
    uint8_t flags7;       /**< Flags byte 7: console type, iNES 2.0 marker, upper mapper nibble. */
    uint8_t flags8;       /**< Flags byte 8: PRG RAM size (rarely used). */
    uint8_t flags9;       /**< Flags byte 9: TV system (NTSC / PAL). */
    uint8_t flags10;      /**< Flags byte 10: unofficial extended flags (bus conflicts, PRG RAM). */
} DiskINes;

/**
 * @brief Identifies the format of the loaded ROM file.
 *
 * Currently only the iNES format is supported.  DISK_ERR is returned when
 * the file cannot be recognised or parsed.
 */
typedef enum DiskFormat {
    DISK_ERR,  /**< The ROM file could not be parsed (bad magic, truncated data, etc.). */
    DISK_INES, /**< Standard iNES format (.nes files). */
} DiskFormat;

/**
 * @brief Parse the iNES header from raw ROM bytes.
 *
 * Reads the 16-byte iNES header starting at @p rom, validates the magic
 * bytes ("NES\x1A"), and fills in @p self with the decoded fields.
 *
 * @param self  Output struct to fill with the parsed header data.
 * @param rom   Pointer to the start of the raw ROM data.
 * @return      true on success, false if the header is invalid.
 */
bool disk_ines_parse(DiskINes *self, const uint8_t *rom);

typedef struct Bus Bus;

/**
 * @brief Represents a loaded NES cartridge.
 *
 * A Disk holds everything that came with the physical cartridge: the ROM data
 * itself, pointers into the PRG and CHR regions, optional save RAM, and the
 * active mapper with all its read/write hooks.
 *
 * Only one Disk can be loaded at a time per Bus.  Call @ref disk_load (or
 * @ref disk_load_mem) to initialise it and @ref disk_unload when done.
 */
typedef struct Disk {
    Bus *bus; /**< The system bus this cartridge is plugged into. */

    DiskFormat fmt; /**< Which ROM file format was detected when loading. */

    /** Format-specific header data.  Only the member matching @ref fmt is valid. */
    union {
        DiskINes ines; /**< Parsed iNES header (valid when fmt == DISK_INES). */
    };

    MapperId mapper; /**< Numeric mapper ID decoded from the ROM header (e.g. 0, 2, 4). */

    /** Active mapper state.  Only the member matching @ref mapper is valid. */
    union {
        Mapper0 map0; /**< Mapper 0 (NROM) – no bank switching, simplest games. */
        Mapper2 map2; /**< Mapper 2 (UxROM) – bank-switchable PRG ROM. */
        Mapper4 map4; /**< Mapper 4 (MMC3) – advanced bank switching + IRQ counter. */
    };

    uint8_t *rom;      /**< Pointer to the full raw ROM data in memory. */
    unsigned  size;    /**< Total size of the ROM data in bytes. */

    uint8_t *prg;      /**< Pointer to the start of the PRG ROM region inside @ref rom. */
    unsigned  prg_size;/**< Size of the PRG ROM region in bytes. */

    uint8_t *prg_ram;  /**< Pointer to PRG RAM (battery-backed save RAM), or NULL if none. */

    uint8_t *chr;      /**< Pointer to CHR data (tile graphics).
                         *   Points into @ref rom for CHR ROM cartridges, or into a
                         *   separately allocated buffer for CHR RAM cartridges. */
    unsigned  chr_size;/**< Size of the CHR region in bytes. */
    bool chr_is_ram;   /**< True if CHR data is writable RAM (the game generates its own
                         *   graphics at runtime), false if it is read-only ROM. */

    Mirroring mirroring; /**< Current nametable mirroring mode.  May be changed at
                           *   runtime by mappers that support switchable mirroring. */

    /* -----------------------------------------------------------------
     * Mapper hook function pointers
     *
     * These are set during disk_load to point at the correct mapper
     * implementation.  The CPU and PPU call these instead of going
     * directly to the mapper structs, so the rest of the emulator does
     * not need to know which mapper is active.
     * ----------------------------------------------------------------- */

    MapperWriteFn    mapper_write;     /**< Called when the CPU writes to cartridge space ($8000-$FFFF). */
    MapperReadFn     mapper_read;      /**< Called when the CPU reads from cartridge space. */
    MapperPpuReadFn  mapper_ppu_read;  /**< Called when the PPU reads from the CHR address space. */
    MapperPpuWriteFn mapper_ppu_write; /**< Called when the PPU writes to the CHR address space (CHR RAM only). */
    MapperHsyncFn    mapper_hsync;     /**< Called at the end of each PPU scanline (used by MMC3 IRQ counter). */
} Disk;

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Load a ROM file from the filesystem into a Disk instance.
 *
 * Opens the file at @p path using the provided filesystem abstraction @p fs,
 * reads it into memory, parses the header, selects the right mapper, and
 * sets up all the read/write hooks.
 *
 * @param self  The Disk struct to initialise.
 * @param bus   The system bus to attach the cartridge to.
 * @param fs    Filesystem handle used to open and read the file.
 * @param path  Path to the .nes ROM file.
 * @return      true on success, false if the file could not be opened or parsed.
 */
bool disk_load(Disk *self, Bus *bus, Fs *fs, const char *path);

/**
 * @brief Load a ROM from a buffer already in memory.
 *
 * Useful when the ROM data has been embedded in firmware or loaded by the
 * caller through some other means.  Behaves identically to @ref disk_load
 * except that no filesystem access is needed.
 *
 * @param self  The Disk struct to initialise.
 * @param bus   The system bus to attach the cartridge to.
 * @param data  Pointer to the raw ROM bytes.
 * @param size  Length of the ROM data in bytes.
 * @return      true on success, false if the data could not be parsed.
 */
bool disk_load_mem(Disk *self, Bus *bus, const uint8_t *data, unsigned size);

/**
 * @brief Unload the cartridge and free any resources it owns.
 *
 * Frees allocated memory (CHR RAM, PRG RAM, the ROM buffer if owned by Disk)
 * and resets the Disk struct so it can be reused with a new @ref disk_load call.
 *
 * @param self  The Disk struct to clean up.
 */
void disk_unload(Disk *self);

#ifdef __cplusplus
}
#endif
