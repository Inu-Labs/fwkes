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

FORCE_INLINE void map2_set_bank(Disk *self, uint8_t bank) {
    uint8_t count = self->map2.prg_bank_count;
    if (count) bank &= (count - 1);
    self->map2.prg_bank = bank;
    self->map2.prg_8000 = &self->prg[bank * 0x4000];
}

void
NOTFLASH_FN(mapper2_write)(Disk *self, uint16_t addr, uint8_t data) {
    switch (addr >> 12) {
    case 0x0:
    case 0x1:
        self->chr[addr] = data;

        break;
    case 0x6:
    case 0x7:
        self->prg_ram[(addr - 0x6000) & (PRG_RAM_SIZE - 1)] = data;

        break;
    case 0x8:
    case 0x9:
    case 0xA:
    case 0xB:
    case 0xC:
    case 0xD:
    case 0xE:
    case 0xF:
        map2_set_bank(self, data);

        break;
    default:
        break;
    }
}

uint8_t NOTFLASH_FN(mapper2_read)(Disk *self, uint16_t addr) {
    switch (addr >> 12) {
    case 0x0:
    case 0x1:
        return self->chr[addr];
    case 0x6:
    case 0x7:
        return self->prg_ram[(addr - 0x6000) & (PRG_RAM_SIZE - 1)];
    case 0x8:
    case 0x9:
    case 0xa:
    case 0xb:
        return self->map2.prg_8000[addr & 0x3fff];
    case 0xc:
    case 0xd:
    case 0xe:
    case 0xf:
        return self->map2.prg_c000[addr & 0x3fff];
    }

    return 0;
}
