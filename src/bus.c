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

#include <fwkes/bus.h>

#include <fwkes/cpu.h>
#include <fwkes/fwx/private.h>
#include <fwkes/log.h>
#include <fwkes/ppu.h>
#include <fwkes/util.h>

#include <string.h>

static void read_disk(Bus *self) {
    uint8_t pc_lsb = bus_read(self, 0xfffc);
    uint8_t pc_msb = bus_read(self, 0xfffd);
    self->cpu.PC = (uint16_t) ((pc_msb << 8) | pc_lsb);

    self->ppu.mirroring = self->disk.mirroring;
    self->ppu.chr = self->disk.chr;

    if (self->ppu.mirroring == MIRRORING_VERTICAL) {
        self->ppu.nt[0] = &self->ppu.vram[0x000];
        self->ppu.nt[1] = &self->ppu.vram[0x400];
        self->ppu.nt[2] = &self->ppu.vram[0x000];
        self->ppu.nt[3] = &self->ppu.vram[0x400];
    } else {
        self->ppu.nt[0] = &self->ppu.vram[0x000];
        self->ppu.nt[1] = &self->ppu.vram[0x000];
        self->ppu.nt[2] = &self->ppu.vram[0x400];
        self->ppu.nt[3] = &self->ppu.vram[0x400];
    }
}

void bus_queue_init(BusQueue *self) { memset(self, 0, sizeof(*self)); }

void bus_queue_clear(BusQueue *self) {
    memset(self, 0, sizeof(*self));
}

void bus_queue_add(BusQueue *self, const BusEvent *ev) {
    if (self->count > BUS_EVENT_QUEUE_CAP) {
        return;
    } else if (self->count == 0) {
        self->head = 0;
        self->tail = 0;
    } else {
        self->tail = (self->tail + 1) % BUS_EVENT_QUEUE_CAP;
    }

    ++self->count;
    memcpy(&self->events[self->tail], ev, sizeof(*ev));
}

BusEvent *bus_queue_pop(BusQueue *self, BusEvent *out) {
    if (self->count == 0) {
        return NULL;
    }

    BusEvent *event = NULL;

    if (self->head == self->tail) {
        event = &self->events[self->head];
        self->head = 0;
        self->tail = 0;
        self->count = 0;
    } else {
        event = &self->events[self->head];
        self->head = (self->head + 1) % BUS_EVENT_QUEUE_CAP;
        --self->count;
    }

    if (out) {
        memcpy(out, event, sizeof(*out));
    }

    return event;
}

bool bus_queue_peek(BusQueue *self, BusEvent *out) {
    if (self->count == 0) {
        return false;
    }

    memcpy(out, &self->events[self->head], sizeof(*out));

    return true;
}

BusEvent *bus_queue_peek_ref(BusQueue *self) {
    if (self->count == 0) {
        return NULL;
    }

    return &self->events[self->head];
}

bool bus_init(Bus *self, Fs *fs) {
    memset(self, 0, sizeof(*self));

    cpu_init(&self->cpu, self);
    ppu_init(&self->ppu, self);
    apu_init(&self->apu, self);
    joypad_reset(&self->joypad1);
    joypad_reset(&self->joypad2);

    bus_queue_init(&self->ev_queue);
    self->fs = fs;

    return fwx_init(&self->fwx, fs, self);
}

void bus_deinit(Bus *self) {
    fwx_deinit(&self->fwx);

    if (self->disk_connected) {
        disk_unload(&self->disk);
    }
}

void bus_reset(Bus *self) {
    bus_queue_clear(&self->ev_queue);

    cpu_reset(&self->cpu);
    ppu_reset(&self->ppu);
    apu_reset(&self->apu);
    joypad_reset(&self->joypad1);
    joypad_reset(&self->joypad2);
    ppu_init_pixel_luts(&self->ppu);

    if (self->disk_connected) {
        read_disk(self);
    }

    if (self->reset_cb) {
        self->reset_cb(self);
    }
}

static bool load_rom_mem(Bus *self, const uint8_t *data, unsigned size) {
    /* To prevent trying to load an invalid .nes file when the currently
     * connected disk is already unloaded, we will load the file into a
     * temporary disk instance. */
    static Disk disk;

    if (!disk_load_mem(&disk, self, data, size)) {
        return false;
    }

    if (self->disk_connected) {
        disk_unload(&self->disk);
    }

    memcpy(&self->disk, &disk, sizeof(disk));
    self->disk_connected = true;

    bus_reset(self);
    read_disk(self);

    return true;
}

static bool NOTFLASH_FN(handle_event)(Bus *self, const BusEvent *ev) {
    switch (ev->id) {
    case BUS_EVENT_NONE:
        break;
    case BUS_EVENT_LOAD_ROM: {
        if (!bus_load_disk(self, ev->load_rom.path)) {
            BusEvent new_ev = {
                .id = BUS_EVENT_SET_FWX_ERR,
                .set_fwx_err = {.err = FWX_ERR_BAD_DATA},
            };

            bus_add_event(self, &new_ev);
        }

        return true;
    }
    case BUS_EVENT_SET_FWX_ERR:
        fwx_set_error(&self->fwx, ev->set_fwx_err.err);

        break;
    case BUS_EVENT_RESET: {
        /* TODO: handle failure of fwx_reset() */
        fwx_reset(&self->fwx);

#ifdef BUILD_RP2350
        bus_load_disk(self, "/bios.nes");
#else
        bus_load_disk(self, "bios.nes");
#endif

        return true;
    }
    }

    return false;
}

bool NOTFLASH_FN(bus_update)(Bus *self) {
    while (self->ev_queue.count > 0) {
        BusEvent *ev = bus_queue_peek_ref(&self->ev_queue);

        if (ev && handle_event(self, ev)) {
            return true;
        }

        bus_queue_pop(&self->ev_queue, NULL);
    }

    return false;
}

void bus_add_event(Bus *self, const BusEvent *ev) {
    bus_queue_add(&self->ev_queue, ev);
}

void NOTFLASH_FN(bus_write)(Bus *self, uint16_t addr, uint8_t data) {
    /* NES memory is organized in 4 KB pages */

    switch (addr >> 12) {
    case 0x0:
    case 0x1:
        /* Internal 2 KiB of RAM */

        self->memory[addr & (0x0800 - 1)] = data;

        break;
    case 0x2:
    case 0x3:
        /* PPU registers, mirrored */

        if (self->ppu.cycles < self->cpu.cycles * 3) {
            ppu_run_until(&self->ppu, self->cpu.cycles * 3);
            ppu_sync(&self->ppu);
        }

        addr = 0x2000 + (addr & 7);
        ppu_write_reg(&self->ppu, addr, data);

        break;
    case 0x4:
        if (addr == 0x4014) {
            uint16_t high_addr = (uint16_t) (data << 8);

            for (uint16_t i = 0; i < 256; ++i) {
                ppu_oam_write(&self->ppu, bus_read(self, high_addr + i));
            }

            if (self->cpu.cycles & 1) {
                self->cpu.cycles += 514;
            } else {
                self->cpu.cycles += 513;
            }
        }

        if (addr == CONTROLLER_POLL_ADDR) {
            joypad_write(&self->joypad1, data);
            joypad_write(&self->joypad2, data);
        } else if (addr >= 0x4000 && addr <= 0x4017) {
            /* APU registers */
            apu_run_until(&self->apu, self->cpu.cycles);
            apu_write(&self->apu, addr, data);
        } else if (addr >= 0x4018 && addr <= 0x401f) {
            /* Disabled APU and I/O functionality, FWX */

            fwx_write(&self->fwx, addr, data);
        }

        break;
    default:
        /* 0x4020..0xffff: unmapped/for cartridge use. */

        self->disk.mapper_write(&self->disk, addr, data);
    }
}

uint8_t NOTFLASH_FN(bus_read)(Bus *self, uint16_t addr) {
    /* NES memory is organized in 4 KB pages. */

    switch (addr >> 12) {
    case 0x0:
    case 0x1:
        /* Internal 2 KiB of RAM */

        return self->memory[addr & (0x0800 - 1)];
    case 0x2:
    case 0x3:
        /* PPU registers, mirrored */

        if (self->ppu.cycles < self->cpu.cycles * 3) {
            ppu_run_until(&self->ppu, self->cpu.cycles * 3);
            ppu_sync(&self->ppu);
        }

        return ppu_read_reg(&self->ppu, 0x2000 + (addr & (8 - 1)));
    case 0x4:
        if (addr == CONTROLLER1_ADDR) {
            /* 0x4016: controller 1 */

            return joypad_read(&self->joypad1);
        } else if (addr == CONTROLLER2_ADDR) {
            /* 0x4017: controller 2 */

            return joypad_read(&self->joypad2);
            // return controller_read(&self->controller2);
        } else if (addr >= 0x4000 && addr <= 0x4017) {
            /* APU registers */
            apu_run_until(&self->apu, self->cpu.cycles);
            return apu_read(&self->apu, addr);
        } else if (addr >= 0x4018 && addr <= 0x401f) {
            /* Disabled APU and I/O functionality, FWX */

            return fwx_read(&self->fwx, addr);
        }

        break;
    default:
        /* 0x4020..0xffff: unmapped/for cartridge use. */

        return self->disk.mapper_read(&self->disk, addr);
    }

    return 0;
}

bool bus_load_disk(Bus *self, const char *path) {
    /* To prevent trying to load an invalid .nes file when the currently
     * connected disk is already unloaded, we will load the file into a
     * temporary disk instance. */
    static Disk disk;

    if (!disk_load(&disk, self, self->fs, path)) {
        return false;
    }

    if (self->disk_connected) {
        disk_unload(&self->disk);
    }

    memcpy(&self->disk, &disk, sizeof(disk));
    self->disk_connected = true;

    bus_reset(self);

    return true;
}

bool bus_load_disk_mem(Bus *self, const uint8_t *data, unsigned size) {
    return load_rom_mem(self, data, size);
}

void bus_hsync(Bus *self) {
    if (self->disk.mapper_hsync) {
        self->disk.mapper_hsync(&self->disk);
    }
}
