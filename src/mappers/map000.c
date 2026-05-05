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

#include <fwkes/mappers/map000.h>

#include <fwkes/disk.h>

void NOTFLASH_FN(mapper0_write)(Disk *disk, uint16_t addr, uint8_t data) {
    switch (addr >> 12) {
    case 0x0:
    case 0x1:
        disk->chr[addr] = data;
        break;

    case 0x6:
    case 0x7:
        disk->prg_ram[(addr - 0x6000) & (PRG_RAM_SIZE - 1)] = data;
        break;

    case 0x8:
    case 0x9:
    case 0xA:
    case 0xB:
    case 0xC:
    case 0xD:
    case 0xE:
    case 0xF:
        break;
    }
}

uint8_t NOTFLASH_FN(mapper0_peek)(const Disk *disk, uint16_t addr) {
    switch (addr >> 12) {
    case 0x0:
    case 0x1:
        return disk->chr[addr];

    case 0x6:
    case 0x7:
        return disk->prg_ram[(addr - 0x6000) & (PRG_RAM_SIZE - 1)];

    case 0x8:
    case 0x9:
    case 0xA:
    case 0xB:
        return disk->map0.prg_8000[addr & 0x3FFF];

    case 0xC:
    case 0xD:
    case 0xE:
    case 0xF:
        return disk->map0.prg_c000[addr & 0x3FFF];
    }

    return 0;
}

void NOTFLASH_FN(mapper0_ppu_write)(Disk *self, uint16_t addr, uint8_t data) {
    (void) self;
    (void) addr;
    (void) data;
}

uint8_t NOTFLASH_FN(mapper0_ppu_peek)(const Disk *self, uint16_t addr) {
    (void) self;
    (void) addr;
    return 0;
}

void NOTFLASH_FN(mapper0_hsync)(Disk *self) { (void) self; }
