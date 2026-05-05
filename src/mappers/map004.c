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

#include <fwkes/mappers/map004.h>

#include <fwkes/bus.h>
#include <fwkes/cpu.h>
#include <fwkes/disk.h>
#include <fwkes/ppu.h>

void mapper4_update_prg(Disk *d) {
    Mapper4 *m = &d->map4;
    uint8_t *prg = d->prg;

    uint8_t last = m->prg_bank_count - 1;
    uint8_t second_last = (m->prg_bank_count >= 2) ? (last - 1) : 0;

    uint8_t mask = m->prg_bank_count - 1;

    uint8_t bank6 = m->bank_regs[6] & mask;
    uint8_t bank7 = m->bank_regs[7] & mask;

    if (m->prg_mode == 0) {
        m->prg_banks[0] = &prg[bank6 << 13];
        m->prg_banks[1] = &prg[bank7 << 13];
        m->prg_banks[2] = &prg[second_last << 13];
        m->prg_banks[3] = &prg[last << 13];
    } else {
        m->prg_banks[0] = &prg[second_last << 13];
        m->prg_banks[1] = &prg[bank7 << 13];
        m->prg_banks[2] = &prg[bank6 << 13];
        m->prg_banks[3] = &prg[last << 13];
    }
}

void mapper4_update_chr(Disk *d) {
    Mapper4 *m = &d->map4;
    uint8_t *chr = d->chr;

    int count = d->chr_size >> 10;
    if (count == 0) count = 1;

    int mask = count - 1;

    int r0 = (m->bank_regs[0] & ~1) & mask;
    int r1 = (m->bank_regs[1] & ~1) & mask;

    int r2 = m->bank_regs[2] & mask;
    int r3 = m->bank_regs[3] & mask;
    int r4 = m->bank_regs[4] & mask;
    int r5 = m->bank_regs[5] & mask;

    if (m->chr_mode == 0) {
        m->chr_banks[0] = &chr[(r0 + 0) << 10];
        m->chr_banks[1] = &chr[(r0 + 1) << 10];

        m->chr_banks[2] = &chr[(r1 + 0) << 10];
        m->chr_banks[3] = &chr[(r1 + 1) << 10];

        m->chr_banks[4] = &chr[r2 << 10];
        m->chr_banks[5] = &chr[r3 << 10];
        m->chr_banks[6] = &chr[r4 << 10];
        m->chr_banks[7] = &chr[r5 << 10];
    } else {
        m->chr_banks[0] = &chr[r2 << 10];
        m->chr_banks[1] = &chr[r3 << 10];
        m->chr_banks[2] = &chr[r4 << 10];
        m->chr_banks[3] = &chr[r5 << 10];

        m->chr_banks[4] = &chr[(r0 + 0) << 10];
        m->chr_banks[5] = &chr[(r0 + 1) << 10];

        m->chr_banks[6] = &chr[(r1 + 0) << 10];
        m->chr_banks[7] = &chr[(r1 + 1) << 10];
    }
}

void NOTFLASH_FN(mapper4_write)(Disk *d, uint16_t addr, uint8_t data) {
    Mapper4 *m = &d->map4;

    if (addr >= 0x6000 && addr < 0x8000) {
        d->prg_ram[(addr - 0x6000) & (PRG_RAM_SIZE - 1)] = data;
        return;
    }

    switch (addr & 0xE001) {

    case 0x8000:
        m->bank_select = data & 0x07;
        m->prg_mode = (data >> 6) & 1;
        m->chr_mode = (data >> 7) & 1;
        break;
    case 0x8001:
        m->bank_regs[m->bank_select] = data;
        mapper4_update_prg(d);
        mapper4_update_chr(d);
        break;

    case 0xA000:
        d->mirroring = (data & 1) ? MIRRORING_HORIZONTAL : MIRRORING_VERTICAL;
        break;

    case 0xC000:
        m->irq_latch = data;

        break;
    case 0xC001:
        m->irq_cnt = 0;
        m->irq_reload_pending = true;

        break;
    case 0xE000:
        m->irq_enabled = false;
        cpu_irq_pulldown(&d->bus->cpu, IRQ_MMC3, false);

        break;
    case 0xE001:
        m->irq_enabled = true;

        break;
    }
}

uint8_t NOTFLASH_FN(mapper4_peek)(const Disk *d, uint16_t addr) {
    const Mapper4 *m = &d->map4;

    if (addr >= 0x8000) {
        uint16_t rel = addr - 0x8000;
        return m->prg_banks[(rel >> 13) & 3][rel & 0x1FFF];
    }

    if (addr >= 0x6000)
        return d->prg_ram[(addr - 0x6000) & (PRG_RAM_SIZE - 1)];

    return 0;
}

uint8_t NOTFLASH_FN(mapper4_ppu_peek)(const Disk *d, uint16_t addr) {
    const Mapper4 *m = &d->map4;
    return m->chr_banks[(addr >> 10) & 7][addr & 0x3FF];
}

void
NOTFLASH_FN(mapper4_ppu_write)(Disk *d, uint16_t addr, uint8_t data) {
    if (d->chr_is_ram) {
        Mapper4 *m = &d->map4;
        int bank = (addr >> 10) & 7;
        m->chr_banks[bank][addr & 0x3FF] = data;
    }
}

void NOTFLASH_FN(mapper4_hsync)(Disk *self) {
    Bus *bus = self->bus;
    Mapper4 *m = &self->map4;
    // uint8_t old_cnt = m->irq_cnt;

    if (m->irq_cnt == 0 || m->irq_reload_pending) {
        m->irq_cnt = m->irq_latch;
        m->irq_reload_pending = false;
    } else {
        if (--m->irq_cnt == 0 && m->irq_enabled) {
            cpu_irq_pulldown(&bus->cpu, IRQ_MMC3, true);
        }
    }
}
