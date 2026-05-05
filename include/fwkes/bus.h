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

#include "apu.h"
#include "cpu.h"
#include "joypad.h"
#include "disk.h"
#include "fs.h"
#include "fwx/private.h"
#include "ppu.h"
#include "joyplayer.h"


#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MEMORY_SIZE 0x10000 /* 65536 bytes */
#define ZEROPAGE_SIZE 0x100 /* 256 bytes */

#define BUS_EVENT_QUEUE_CAP 32

typedef enum BusEventId {
    BUS_EVENT_NONE,
    BUS_EVENT_LOAD_ROM,
    BUS_EVENT_SET_FWX_ERR,
    BUS_EVENT_RESET
} BusEventId;

typedef struct BusEventLoadRom {
    /* must be a valid string until the event is processed!!! */
    const char *path;
} BusEventLoadRom;

typedef struct BusSetFwxError {
    FwxError err;
} BusSetFwxError;

typedef struct BusEvent {
    BusEventId id;

    union {
        BusEventLoadRom load_rom;
        BusSetFwxError set_fwx_err;
    };
} BusEvent;

typedef struct BusQueue {
    BusEvent events[BUS_EVENT_QUEUE_CAP];
    unsigned count;
    unsigned head, tail;
} BusQueue;

void bus_queue_init(BusQueue *self);
void bus_queue_clear(BusQueue *self);
void bus_queue_add(BusQueue *self, const BusEvent *ev);
BusEvent *bus_queue_pop(BusQueue *self, BusEvent *out);
BusEvent *bus_queue_peek_ref(BusQueue *self);
bool bus_queue_peek(BusQueue *self, BusEvent *out);

typedef void (*BusResetCallback)(Bus *bus);

typedef enum BusErrorId {
    BUS_ERROR_OK,
    BUS_ERROR_FWX,
} BusErrorId;

typedef struct BusError {
    BusErrorId id;
    union {
        FwxError err;
    };
} BusError;

typedef struct Bus {
    Cpu cpu;
    Ppu ppu;
    Disk disk;
    Joypad joypad1;
    Joypad joypad2;
    Apu apu;
    Fwx fwx;
    Fs *fs;
    uint8_t memory[0x800];
    bool disk_connected;
    BusQueue ev_queue;
    void *user_data;
    BusResetCallback reset_cb;
    bool reset_requested;
    JoyPlayer joyplayer;
    char bios_path[256];
} Bus;

bool bus_init(Bus *self, Fs *fs, const char *bios_path);
void bus_deinit(Bus *self);
void bus_reset(Bus *self);
void bus_unload_disk(Bus *self);
void bus_update(Bus *self);
void bus_add_event(Bus *self, const BusEvent *ev);
void bus_write(Bus *self, uint16_t address, uint8_t data);
uint8_t bus_read(Bus *self, uint16_t address);
uint8_t bus_peek(const Bus *self, uint16_t address);
bool bus_load_disk(Bus *self, const char *path);
bool bus_load_disk_mem(Bus *self, const uint8_t *data, unsigned size);
void bus_hsync(Bus *self);

#ifdef __cplusplus
}
#endif
