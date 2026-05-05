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

#include "disk.h"
#include "util.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <fwkes/disk.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PPU_CTRL_NAMETABLE_ADDR 0x3
#define PPU_CTRL_INC_MODE (1u << 2)
#define PPU_CTRL_SPR_TILE_SELECT (1u << 3)
#define PPU_CTRL_BG_TILE_SELECT (1u << 4)
#define PPU_CTRL_SPR_HEIGHT (1u << 5)
#define PPU_CTRL_MS (1u << 6)
#define PPU_CTRL_NMI_ON (1u << 7)

#define PPU_MASK_GREYSCALE (1u << 0)
#define PPU_MASK_BG_LEFT_COL_ON (1u << 1)
#define PPU_MASK_SPR_LEFT_COL_ON (1u << 2)
#define PPU_MASK_BG_ON (1u << 3)
#define PPU_MASK_SPR_ON (1u << 4)
#define PPU_MASK_EMPH_RED (1u << 5)
#define PPU_MASK_EMPH_GREEN (1u << 6)
#define PPU_MASK_EMPH_BLUE (1u << 7)

#define PPU_STAT_OVERFLOW (1u << 5)
#define PPU_STAT_ZERO_HIT (1u << 6)
#define PPU_STAT_VBLANK (1u << 7)

#define PPU_WIDTH 256
#define PPU_HEIGHT 240

#define PPU_CTRL 0x2000
#define PPU_MASK 0x2001
#define PPU_STAT 0x2002
#define PPU_SCROLL 0x2005
#define PPU_ADDR 0x2006
#define PPU_DATA 0x2007

#define OAM_ADDR 0x2003
#define OAM_DATA 0x2004
#define OAM_DMA 0x4014

#define PPU_PT0 0x0000
#define PPU_PT1 0x1000
#define PPU_NT0 0x2000
#define PPU_AT0 0x23c0
#define PPU_NT1 0x2400
#define PPU_AT1 0x27c0
#define PPU_NT2 0x2800
#define PPU_AT2 0x2bc0
#define PPU_NT3 0x2c00
#define PPU_AT3 0x2fc0
#define PPU_BG_PALLETE0 0x3f00
#define PPU_BG_PALLETE1 0x3f04
#define PPU_BG_PALLETE2 0x3f08
#define PPU_BG_PALLETE3 0x3f0c
#define PPU_SPR_PALLETE0 0x3f10
#define PPU_SPR_PALLETE1 0x3f14
#define PPU_SPR_PALLETE2 0x3f18
#define PPU_SPR_PALLETE3 0x3f1c

#define VRAM_COARSE_X 0x1f
#define VRAM_COARSE_Y 0x3e0
#define VRAM_NT_SEL 0xc00
#define VRAM_NTX_SEL 0x400
#define VRAM_NTY_SEL 0x800
#define VRAM_FINE_Y 0x7000

#define vram_get_coarse_x(v) ((v) & VRAM_COARSE_X)
#define vram_get_coarse_y(v) (((v) & VRAM_COARSE_Y) >> 5)
#define vram_get_nametable(v) (((v) & VRAM_NT_SEL) >> 10)
#define vram_get_fine_y(v) (((v) & VRAM_FINE_Y) >> 12)

/*
 * V is increment during rendering:
 *   - coarse X increment: every tile fetch (=each multiply of 8 cycles)
 *   - fine Y & coarse Y increment: at end of a scanline
 *
 * T & V relationship:
 *   - during rendering, at specific dots (257, 280-304), v = t
 *
 * X during rendering:
 *   - used to shift the fetched tile pixels horizontally
 */

typedef struct Bus Bus;
typedef struct Ppu Ppu;

typedef void (*PpuScanlineCallback)(Ppu *self);

typedef enum ScanlineId {
    SCANLINE_PRE_RENDER,
    SCANLINE_VISIBLE,
    SCANLINE_VBLANK,
    SCANLINE_POST_RENDER
} ScanlineId;

typedef uint32_t TileRow;

#ifdef BUILD_RP2350
typedef uint16_t PpuPixel;
#else
typedef uint32_t PpuPixel;
#endif

typedef struct Sprite {
    uint8_t pos_y;
    TileRow tile;
    uint8_t attr;
    uint8_t pos_x;
    bool is_0;
} Sprite;

typedef struct Ppu {
    Bus *bus;

    uint8_t ctrl;
    uint8_t mask;
    uint8_t status;
    uint16_t scroll;
    uint8_t data;
    uint8_t oam_addr;

    uint16_t v;
    uint16_t t;
    uint8_t x;
    uint8_t w;

    Sprite queued_sprites[8];
    unsigned queued_sprites_count;

    // uint8_t bg_opaque[256];
    // uint8_t scanline_buf[PPU_WIDTH];
    uint8_t scanline_buf[PPU_WIDTH];
    PpuScanlineCallback scanline_cb;
    void *user_data;
    const PpuPixel *colors;
    PpuPixel colors_lut[32];
    bool colors_lut_dirty; /* for RP2350 */

    uint8_t vram[0x800];
    uint8_t oam[0x100];
    uint8_t palettes[32];
    uint8_t *chr;
    uint8_t temp_read_buf;
    Mirroring mirroring;
    unsigned scanline;
    unsigned dot;
    CycleCounter cycles;
    unsigned last_rendered_dot;
    bool frame_done;
    TileRow cached_tile;
    uint8_t *nt[4];
    // bool odd_frame;
} Ppu;

uint8_t ppu_read_reg(Ppu *self, uint16_t addr);
uint8_t ppu_peek_reg(const Ppu *self, uint16_t reg);
void ppu_write_reg(Ppu *self, uint16_t addr, uint8_t data);
void ppu_write(Ppu *self, uint16_t addr, uint8_t data);
uint8_t ppu_read(Ppu *self, uint16_t addr);
uint8_t ppu_peek(const Ppu *self, uint16_t addr);
void ppu_oam_write(Ppu *self, uint8_t data);

void ppu_init(Ppu *self, Bus *bus);
void ppu_init_pixel_luts(Ppu *self);
void ppu_reset(Ppu *self);
void ppu_sync(Ppu *self);
void ppu_run_until(Ppu *self, CycleCounter target_cycle);
void ppu_scanline_end(Ppu *self);
void ppu_render_scanline(Ppu *self);

#ifdef __cplusplus
}
#endif
