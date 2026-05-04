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

#include <fwkes/disk.h>

#include <fwkes/bus.h>
#include <fwkes/log.h>
#include <fwkes/util.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef BUILD_RP2350
#    include <fwkes/rp2350/psram.h>
#    define disk_malloc(size) p_malloc(size)
#    define disk_calloc(num, size) p_calloc(num, size)
#    define disk_free(block) p_free(block)
#else
#    define disk_malloc(size) malloc(size)
#    define disk_calloc(num, size) calloc(num, size)
#    define disk_free(block) free(block)
#endif

static void *load_file(Fs *fs, const char *path, unsigned *size) {
    File file;

    if (fs_open(fs, &file, path, "rb") != FS_ERR_OK) {
        return NULL;
    }

    size_t size_tmp;
    if (file_size(&file, &size_tmp) != FS_ERR_OK) {
        file_close(&file);
        return NULL;
    }

    *size = (unsigned) size_tmp;

    void *buf = disk_malloc(*size);

    if (!buf) {
        file_close(&file);

        return NULL;
    }

    if (file_read(&file, buf, *size) != FS_ERR_OK) {
        disk_free(buf);
        buf = NULL;
    }

    file_close(&file);

    return buf;
}

FORCE_INLINE MapperId mapper(const Disk *self) {
    switch (self->fmt) {
    case DISK_ERR:
        return 0;
    case DISK_INES:
        return ((self->ines.flags6 & INES_FLAGS6_MAPPER_LN) >> 4) |
               (self->ines.flags7 & INES_FLAGS7_MAPPER_HN);
    }

    return 0;
}

FORCE_INLINE unsigned prg_size(const Disk *self) {
    switch (self->fmt) {
    case DISK_ERR:
        return 0;
    case DISK_INES:
        return self->ines.prg_rom_size * PRG_ROM_UNIT;
    }

    return 0;
}

FORCE_INLINE unsigned chr_size(const Disk *self) {
    switch (self->fmt) {
    case DISK_ERR:
        return 0;
    case DISK_INES:
        return self->ines.chr_rom_size * CHR_ROM_UNIT;
    }

    return 0;
}

bool disk_ines_parse(DiskINes *self, const uint8_t *rom) {
    if (memcmp(rom, "NES\x1a", 4) != 0) {
        log_msg(
            LOG_WARN,
            "bad iNES/NES 2.0 signature: 0x%02x 0x%02x 0x%02x 0x%02x (must be "
            "0x%02x 0x%02x 0x%02x 0x%02x)",
            rom[0], rom[1], rom[2], rom[3], 'N', 'E', 'S', '\x1a'
        );

        return false;
    }

    self->prg_rom_size = rom[4];
    self->chr_rom_size = rom[5];
    self->flags6 = rom[6];
    self->flags7 = rom[7];
    self->flags8 = rom[8];
    self->flags9 = rom[9];
    self->flags10 = rom[10];

    return true;
}

static bool parse(Disk *self) {
    if (disk_ines_parse(&self->ines, self->rom)) {
        self->fmt = DISK_INES;
    } else {
        disk_unload(self);

        return false;
    }

    self->mapper = mapper(self);
    size_t offset = 0x10;

    if (self->ines.flags6 & 0x04) {
        offset += 512;
    }

    self->prg_size = prg_size(self);
    self->chr_size = chr_size(self);

    self->prg = &self->rom[offset];
    self->chr = &self->rom[offset + self->prg_size];
    self->prg_ram = disk_calloc(1, PRG_RAM_SIZE);
    self->mirroring = (self->ines.flags6 & INES_FLAGS6_NAMETABLE_MIRRORING)
                          ? MIRRORING_VERTICAL
                          : MIRRORING_HORIZONTAL;

    if (!self->prg_ram) {
        disk_unload(self);

        return false;
    }

    switch (self->mapper) {
    case 0:
        self->map0.prg_c000_addr =
            self->prg_size == PRG_ROM_UNIT * 1 ? 0 : 0x4000;
        self->map0.prg_8000 = &self->prg[0];
        self->map0.prg_c000 = &self->prg[self->map0.prg_c000_addr];

        self->mapper_write = mapper0_write;
        self->mapper_read = mapper0_read;
        self->mapper_ppu_read = NULL;
        self->mapper_ppu_write = NULL;
        self->mapper_hsync = NULL;

        break;
    case 2:
        if (self->chr_size == 0) {
            self->chr_is_ram = true;
            self->chr_size = 0x2000;
            self->chr = disk_calloc(1, 0x2000);
        }

        self->map2.prg_bank_count = (uint8_t) (self->prg_size >> 14);
        self->map2.prg_bank = 0;
        self->map2.prg_8000 = &self->prg[0];
        self->map2.prg_c000 =
            &self->prg[(self->map2.prg_bank_count - 1) * 0x4000];

        self->mapper_write = mapper2_write;
        self->mapper_read = mapper2_read;
        self->mapper_ppu_read = NULL;
        self->mapper_ppu_write = NULL;
        self->mapper_hsync = NULL;

        break;

    case 4:
        if (self->chr_size == 0) {
            self->chr_is_ram = true;
            self->chr_size = 0x2000;
            self->chr = disk_calloc(1, 0x2000);
        }

        Mapper4 *m = &self->map4;

        memset(m, 0, sizeof(Mapper4));

        m->prg_bank_count = self->prg_size / 0x2000;
        if (m->prg_bank_count == 0) {
            m->prg_bank_count = 1;
        }

        m->irq_cnt = 0;
        m->irq_enabled = false;
        m->irq_reload_pending = false;
        m->irq_latch = 0;

        mapper4_update_prg(self);
        mapper4_update_chr(self);

        self->mapper_write = mapper4_write;
        self->mapper_read = mapper4_read;
        self->mapper_ppu_read = mapper4_ppu_read;
        self->mapper_ppu_write = mapper4_ppu_write;
        self->mapper_hsync = mapper4_hsync;

        break;
    default:
        log_msg(LOG_ERROR, "unknown mapper %u\n", self->mapper);
        disk_unload(self);

        return false;
    }

    return true;
}

bool disk_load(Disk *self, Bus *bus, Fs *fs, const char *path) {
    memset(self, 0, sizeof(*self));
    self->rom = load_file(fs, path, &self->size);
    self->bus = bus;

    if (!self->rom) {
        return false;
    }

    return parse(self);
}

bool disk_load_mem(Disk *self, Bus *bus, const uint8_t *data, unsigned size) {
    memset(self, 0, sizeof(*self));
    self->rom = disk_malloc(size);
    self->bus = bus;

    if (!self->rom) {
        return false;
    }

    memcpy(self->rom, data, size);

    return parse(self);
}

void disk_unload(Disk *self) {
    disk_free(self->prg_ram);
    disk_free(self->rom);

    if (self->chr_is_ram) {
        disk_free(self->chr);
    }

    memset(self, 0, sizeof(*self));
}
