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

#include <fwkes/ppu.h>

#include <fwkes/bits.h>
#include <fwkes/bus.h>
#include <fwkes/cpu.h>
#include <fwkes/log.h>
#include <fwkes/trace.h>
#include <fwkes/util.h>

#include <assert.h>
#include <string.h>

#define SCANLINE_LENGTH 341
#define IDX_1D_YX(w, y, x) ((y) * (w) + (x))

static uint32_t g_row_high_lut[256] __attribute__((section(".data")));
static uint32_t g_row_low_lut[256] __attribute__((section(".data")));
static uint32_t g_flipped_row_high_lut[256] __attribute__((section(".data")));
static uint32_t g_flipped_row_low_lut[256] __attribute__((section(".data")));
static uint32_t g_attr_mask_lut[4] __attribute__((section(".data"))) = {
    0x00000000, 0x44444444, 0x88888888, 0xcccccccc
};

FORCE_INLINE uint16_t get_pattern_table_bg(const Ppu *self) {
    return (self->ctrl & PPU_CTRL_BG_TILE_SELECT) ? 0x1000 : 0x0000;
}

FORCE_INLINE uint16_t get_pattern_table_spr(const Ppu *self) {
    return (self->ctrl & PPU_CTRL_SPR_TILE_SELECT) ? 0x1000 : 0x0000;
}

FORCE_INLINE void set_w(Ppu *self, bool v) {
    if (self->w != v)
        trace_ppu(TRACE_W_LATCH, "%d", v);

    self->w = v;
}

FORCE_INLINE void toggle_w(Ppu *self) { set_w(self, !self->w); }

FORCE_INLINE void set_vblank(Ppu *self, bool v) {
    if ((bool) (self->status & PPU_STAT_VBLANK) != v) {
        trace_ppu(TRACE_VBLANK, "%d", v);
    }

    if (v) {
        self->status |= PPU_STAT_VBLANK;
    } else {
        self->status &= ~PPU_STAT_VBLANK;
    }
}

FORCE_INLINE void trigger_nmi(Ppu *self) {
    if (self->ctrl & PPU_CTRL_NMI_ON) {
        trace_ppu(TRACE_NMI, "NMI triggered");
        self->bus->cpu.nmi_pending = true;
    }
}

FORCE_INLINE void set_spr_0_hit(Ppu *self, bool v) {
    if ((self->status & PPU_STAT_ZERO_HIT) ^ v) {
        trace_ppu(TRACE_0_HIT, "%d", v);
    }

    if (v) {
        self->status |= PPU_STAT_ZERO_HIT;
    } else {
        self->status &= ~PPU_STAT_ZERO_HIT;
    }
}

FORCE_INLINE uint16_t vram_idx(Mirroring mirroring, uint16_t addr) {
    uint16_t idx = addr & 0x0fff;

    if (mirroring == MIRRORING_VERTICAL) {
        return idx & 0x07ff;
    } else {
        return (idx & 0x3ff) | ((idx & 0x800) >> 1);
    }
}

FORCE_INLINE void increment_vram_addr(Ppu *self) {
    if (self->ctrl & PPU_CTRL_INC_MODE) {
        self->v += 32;
    } else {
        ++self->v;
    }

    if (self->v > 0x3fff) {
        self->v &= 0x3fff;
    }
}

uint16_t vram_get_mirrored_addr(Mirroring mode, uint16_t addr) {
    addr &= 0x0FFF;
    switch (mode) {
    case MIRRORING_HORIZONTAL:
        return ((addr & 0x0800) >> 1) | (addr & 0x03FF);
    case MIRRORING_VERTICAL:
        return addr & 0x07FF;
    default:
        return addr;
    }
}

void ppu_init(Ppu *self, Bus *bus) {
    memset(self, 0, sizeof(Ppu));
    self->bus = bus;
    self->status = 0xa0;
    self->scanline = 261;
    self->dot = 0;
    self->cycles = 0;

    for (unsigned i = 0; i < 256; ++i) {
        for (unsigned j = 0; j < 8; ++j) {
            if (i & (1 << (7 - j))) {
                g_row_low_lut[i] |= (1ull << (j * 4));
                g_row_high_lut[i] |= (2ull << (j * 4));
            }

            if (i & (1 << j)) {
                g_flipped_row_low_lut[i] |= (1ull << (j * 4));
                g_flipped_row_high_lut[i] |= (2ull << (j * 4));
            }
        }
    }

    static const uint8_t power_on_palette[32] = {
        0x09, 0x01, 0x00, 0x01, 0x00, 0x02, 0x02, 0x0D, 0x08, 0x10, 0x08,
        0x24, 0x00, 0x00, 0x04, 0x2C, 0x09, 0x01, 0x34, 0x03, 0x00, 0x04,
        0x00, 0x14, 0x08, 0x3A, 0x00, 0x02, 0x00, 0x20, 0x2C, 0x08
    };

    for (int i = 0; i < 32; ++i) {
        self->palettes[i] = power_on_palette[i];
    }
}

void ppu_init_pixel_luts(Ppu *self) {
    for (unsigned i = 0; i < 16; ++i) {
        uint8_t color_idx = i & 0x03;
        uint8_t palette_idx = (i >> 2) & 0x03;

        if (color_idx == 0) {
            self->colors_lut[i] = self->colors[self->palettes[0]];
            self->colors_lut[i + 16] = self->colors[self->palettes[0]];
        } else {
            uint8_t pal_entry = (uint8_t) (palette_idx << 2) | color_idx;
            self->colors_lut[i] = self->colors[self->palettes[pal_entry]];
            self->colors_lut[i + 16] =
                self->colors[self->palettes[0x10 | pal_entry]];
        }
    }

    self->colors_lut_dirty = true;
}

void ppu_reset(Ppu *self) {
    self->ctrl = 0x00;
    self->mask = 0x00;
    self->status = 0xa0;
    self->w = 0;
    self->scroll = 0x00;
    self->data = 0x00;
    self->cycles = 0;
    self->scanline = 261;
    self->dot = 0;
}

uint8_t NOTFLASH_FN(ppu_read_reg)(Ppu *self, uint16_t addr) {
    switch (addr) {
    case PPU_CTRL:
        trace_ppu(TRACE_REG_READ, "PPUCTRL = %02x", self->ctrl);
        return self->ctrl;
    case PPU_MASK:
        trace_ppu(TRACE_REG_READ, "PPUMASK = %02x", self->mask);
        return self->mask;
    case PPU_STAT: {
        trace_ppu(TRACE_REG_READ, "PPUSTATUS = %02x", self->status);

        uint8_t old_status = self->status;

        set_w(self, 0);
        set_vblank(self, 0);

        return old_status;
    }
    case OAM_ADDR:
        trace_ppu(TRACE_REG_READ, "OAMADDR = %02x", self->oam_addr);
        return self->oam_addr;
    case OAM_DATA:
        trace_ppu(TRACE_REG_READ, "OAMDATA = %02x", self->oam[self->oam_addr]);
        return self->oam[self->oam_addr];
    case PPU_SCROLL: {
        uint8_t data =
            self->w ? (self->scroll & 0xff) : ((self->scroll & 0xff00) >> 8);

        toggle_w(self);

        trace_ppu(
            TRACE_REG_READ, "PPUSCROLL = %02x (w = %d), %02x (w = %d) (old)",
            self->w ? ((self->scroll & 0xff00) >> 8) : (self->scroll & 0xff),
            self->w, data, !self->w
        );

        return data;
    }
    case PPU_ADDR: {
        uint8_t data = self->w ? (self->v & 0xff) : ((self->v & 0xff00) >> 8);

        toggle_w(self);

        if (!self->w) {
            trace_ppu(TRACE_REG_READ, "PPUADDR:L = %02x", self->v & 0xff);
        } else {
            trace_ppu(
                TRACE_REG_READ, "PPUADDR:H = %02x", (self->v & 0xff00) >> 8
            );
        }

        return data;
    }
    case PPU_DATA:
        addr = self->v;
        increment_vram_addr(self);

        if (addr >= 0x000 && addr <= 0x1fff) {
            uint8_t old_data = self->temp_read_buf;

            Disk *disk = &self->bus->disk;

            if (disk->mapper_ppu_read) {
                self->temp_read_buf = disk->mapper_ppu_read(disk, addr);
            } else {
                self->temp_read_buf = self->chr[addr];
            }

            return old_data;
        } else if (addr >= 0x2000 && addr <= 0x2fff) {
            uint8_t old_data = self->temp_read_buf;
            self->temp_read_buf = self->vram[vram_idx(self->mirroring, addr)];

            trace_ppu(TRACE_REG_READ, "PPUDATA = %02x", old_data);

            return old_data;
        } else if (addr >= 0x3000 && addr <= 0x3eff) {
            log_ppu(LOG_WARN, "attempted to read from unused space");
        } else if (addr >= 0x3f00 && addr <= 0x3f1f) {
            uint8_t data = self->palettes[addr - 0x3f00];

            trace_ppu(TRACE_REG_READ, "PPUDATA = %02x", data);

            return data;
        } else if (addr >= 0x3f20 && addr <= 0x3fff) {
            uint8_t data = self->palettes[addr & 0x1f];

            trace_ppu(TRACE_REG_READ, "PPUDATA = %02x", data);

            return data;
        } else {
            log_ppu(
                LOG_WARN, "attempted to read from unmapped address 0x%04x", addr
            );
        }

        break;
    default:
        return 0x00;
    }

    return 0x00;
}

uint8_t NOTFLASH_FN(ppu_peek_reg)(const Ppu *self, uint16_t reg) {
    switch (reg) {
    case PPU_CTRL:
        return self->ctrl;
    case PPU_MASK:
        return self->mask;
    case PPU_STAT:
        return self->status;
    case OAM_ADDR:
        return self->oam_addr;
    case OAM_DATA:
        return self->oam[self->oam_addr];
    case PPU_SCROLL:
        return self->w ? (self->scroll & 0xff) : ((self->scroll & 0xff00) >> 8);
    case PPU_ADDR:
        return self->w ? (self->v & 0xff) : ((self->v & 0xff00) >> 8);
    case PPU_DATA:
        if (self->v >= 0x000 && self->v <= 0x1fff) {
            return self->chr[self->v];
        } else if (self->v >= 0x2000 && self->v <= 0x2fff) {
            return self->vram[vram_idx(self->mirroring, self->v)];
        } else if (self->v >= 0x3f00 && self->v <= 0x3f1f) {
            return self->palettes[self->v - 0x3f00];
        } else if (self->v >= 0x3f20 && self->v <= 0x3fff) {
            return self->palettes[self->v & 0x1f];
        }

        break;
    default:
        return 0x00;
    }

    return 0x00;
}

void NOTFLASH_FN(ppu_write_reg)(Ppu *self, uint16_t addr, uint8_t data) {
    switch (addr) {
    case PPU_CTRL: {
        /* TODO: consider implementing the "bit 0 race condition" bug */

        uint8_t old_ctrl = self->ctrl;
        trace_ppu(TRACE_REG_WRITE, "PPUCTRL = %02x", data);
        self->ctrl = data;

        self->t &= ~VRAM_NT_SEL;
        self->t |= (data & 0x3) << 10;

        if ((self->status & PPU_STAT_VBLANK) && (data & PPU_CTRL_NMI_ON) &&
            !(old_ctrl & PPU_CTRL_NMI_ON)) {
            trigger_nmi(self);
        }

        break;
    }
    case PPU_MASK:
        trace_ppu(TRACE_REG_WRITE, "PPUMASK = %02x", data);
        self->mask = data;

        break;
    case PPU_STAT:
        break;
    case OAM_ADDR:
        trace_ppu(TRACE_REG_WRITE, "OAMADDR = %02x", data);
        self->oam_addr = data;

        break;
    case OAM_DATA:
        trace_ppu(TRACE_REG_WRITE, "OAMDATA = %02x", data);
        self->oam[self->oam_addr] = data;
        ++self->oam_addr;

        break;
    case PPU_SCROLL:
        if (self->w) {
            /* w=1: vertical scroll */

            self->t &= ~(VRAM_COARSE_Y | VRAM_FINE_Y);
            self->t |= (data & 0x7) << 12; /* fine y */
            self->t |= (data & 0xf8) << 2; /* coarse y */
        } else {
            /* w=0: horizontal scroll */

            self->t &= ~VRAM_COARSE_X;
            self->t |= (data >> 3) & VRAM_COARSE_X /* coarse x */;
            self->x &= ~0x7;
            self->x |= data & 0x7 /* fine x */;
        }

        toggle_w(self);

        break;
    case PPU_ADDR:
        if (self->w) {
            /* w=1: low byte */

            trace_ppu(TRACE_REG_WRITE, "PPUADDR:L = %02x", data);
            self->t = (self->t & ~0x00ff) | data;
            self->v = self->t;
        } else {
            /* w=0: high byte */

            trace_ppu(TRACE_REG_WRITE, "PPUADDR:H = %02x", data);
            self->t = (self->t & ~0xff00) | (uint16_t) ((data & 0x3f) << 8);
        }

        toggle_w(self);

        if (self->v > 0x3fff) {
            self->v &= 0x3fff;
        }

        break;
    case PPU_DATA:
        trace_ppu(
            TRACE_REG_WRITE, "PPUDATA = %02x (addr = %04x)", data, self->v
        );

        ppu_write(self, self->v, data);
        increment_vram_addr(self);

        break;
    case OAM_DMA:
        trace_ppu(TRACE_REG_WRITE, "OAMDATA = %02x", data);
        self->oam[self->oam_addr] = data;
        ++self->oam_addr;

        break;
    default:
        break;
    }
}

void NOTFLASH_FN(ppu_write)(Ppu *self, uint16_t addr, uint8_t data) {
    addr &= 0x3fff;

    if (addr < 0x2000) {
        if (self->bus->disk.mapper_ppu_write) {
            self->bus->disk.mapper_ppu_write(&self->bus->disk, addr, data);
            return;
        }

        if (self->bus->disk.chr_is_ram) {
            self->chr[addr] = data;
        }

        return;
    }

    if (addr < 0x3f00) {
        self->nt[(addr >> 10) & 3][addr & 0x3ff] = data;
        return;
    }

    uint16_t pal_idx = addr & 0x1f;

    if ((pal_idx & 0x13) == 0x10) {
        pal_idx &= ~0x10;
    }

    self->palettes[pal_idx] = data;
    ppu_init_pixel_luts(self);
}

uint8_t NOTFLASH_FN(ppu_read)(Ppu *self, uint16_t addr) {
    addr &= 0x3fff;

    if (addr < 0x2000) {
        if (self->bus->disk.mapper_ppu_read) {
            return self->bus->disk.mapper_ppu_read(&self->bus->disk, addr);
        }

        return self->chr[addr];
    }

    if (addr < 0x3f00) {
        return self->nt[(addr >> 10) & 3][addr & 0x3ff];
    }

    uint16_t pal_addr = addr & 0x1f;

    if ((pal_addr & 0x03) == 0 && (pal_addr & 0x10)) {
        pal_addr &= ~0x10;
    }

    return self->palettes[pal_addr];
}

void NOTFLASH_FN(ppu_oam_write)(Ppu *self, uint8_t data) {
    self->oam[self->oam_addr] = data;
    ++self->oam_addr;
}

FORCE_INLINE uint16_t calc_attr_addr(uint16_t v) {
    uint16_t nt_sel = v & VRAM_NT_SEL;
    uint16_t coarse_x = ((v & VRAM_COARSE_X) & 0x1c) >> 2;
    uint16_t coarse_y = ((v & VRAM_COARSE_Y) & 0x380) >> 4;

    return PPU_AT0 | nt_sel | coarse_x | coarse_y;
}

FORCE_INLINE void increment_coarse_x(Ppu *self) {
    if ((self->v & VRAM_COARSE_X) == 31) {
        self->v &= ~VRAM_COARSE_X;
        self->v ^= 0x0400;
    } else {
        ++self->v;
    }
}

FORCE_INLINE void increment_coarse_y(Ppu *self) {
    if ((self->v & 0x7000) != 0x7000) {
        self->v += 0x1000;
    } else {
        self->v &= ~0x7000;

        uint16_t y = (self->v & 0x03e0) >> 5;

        if (y == 29) {
            y = 0;
            self->v ^= 0x0800;
        } else if (y == 31) {
            y = 0;
        } else {
            y += 1;
        }

        self->v = (uint16_t) (self->v & ~0x03e0) | (uint16_t) (y << 5);
    }
}

/* This function packs 1 tile row into 32-bit data type. Each nibble stores
 * color index and palette index. */
static TileRow NOTFLASH_FN(fetch_tile)(const Ppu *self) {
    uint16_t v = self->v;

    uint16_t addr =
        PPU_NT0 | (v & (VRAM_COARSE_X | VRAM_COARSE_Y | VRAM_NT_SEL));
    uint8_t tile_idx = self->nt[(addr >> 10) & 3][addr & 0x3ff];
    uint16_t attr_addr = calc_attr_addr(v);
    uint8_t attr = self->nt[(attr_addr >> 10) & 3][attr_addr & 0x3ff];

    unsigned shift = ((v >> 4) & 4) | (v & 2);
    uint8_t palette_idx = (attr >> shift) & 3;

    uint16_t row_addr = get_pattern_table_bg(self) +
                        (uint16_t) (tile_idx << 4) + vram_get_fine_y(v);
    Disk *disk = &self->bus->disk;

    uint8_t row_low;
    uint8_t row_high;

    if (disk->mapper_ppu_read) {
        row_low = disk->mapper_ppu_read(disk, row_addr);
        row_high = disk->mapper_ppu_read(disk, row_addr + 8);
    } else {
        row_low = self->chr[row_addr];
        row_high = self->chr[row_addr + 8];
    }

    uint32_t raw_pixels = g_row_high_lut[row_high] | g_row_low_lut[row_low];
    uint32_t attr_mask = g_attr_mask_lut[palette_idx];

    return raw_pixels | attr_mask;
}

/* Just like fetch_tile(), this function packs 1 tile row into 32-bit data type.
 * Each nibble stores color index and palette index. */
static TileRow
NOTFLASH_FN(fetch_spr_tile)(const Ppu *self, uint16_t pat_addr, uint8_t attr) {
    Disk *disk = &self->bus->disk;

    uint8_t row_low;
    uint8_t row_high;

    if (disk->mapper_ppu_read) {
        row_low = disk->mapper_ppu_read(disk, pat_addr);
        row_high = disk->mapper_ppu_read(disk, pat_addr + 8);
    } else {
        row_low = self->chr[pat_addr];
        row_high = self->chr[pat_addr + 8];
    }
    uint32_t raw_pixels;

    /* horizontal flip */
    if (attr & 0x40) {
        raw_pixels =
            g_flipped_row_low_lut[row_low] | g_flipped_row_high_lut[row_high];
    } else {
        raw_pixels = g_row_low_lut[row_low] | g_row_high_lut[row_high];
    }

    uint8_t palette_idx = (attr & 0x03);
    uint32_t attr_mask = g_attr_mask_lut[palette_idx];

    return raw_pixels | attr_mask;
}

/* Bugged sprite overflow detection */
FORCE_INLINE void
check_sprite_overflow(Ppu *self, unsigned sprite_idx, uint8_t h) {
    if (self->queued_sprites_count == 8) {
        /* n ... sprite index (in OAM)
         * m ... byte index (inside sprite) */
        unsigned n = sprite_idx;
        unsigned m = 0;

        while (n < 64) {
            uint8_t y = self->oam[n * 4 + m];
            int16_t row = (int16_t) self->scanline - y;

            if (row >= 0 && row < h) {
                self->status |= PPU_STAT_OVERFLOW;
                m = (m + 1) & 3;

                if (m == 0) {
                    ++n;
                }

                break;
            } else {
                ++n;
                m = (m + 1) & 3;
            }
        }
    }
}

static void NOTFLASH_FN(queue_sprites)(Ppu *self) {
    self->queued_sprites_count = 0;
    uint8_t h = (self->ctrl & PPU_CTRL_SPR_HEIGHT) ? 16 : 8;
    unsigned i = 0;

    for (; i < 64 && self->queued_sprites_count < 8; ++i) {
        uint8_t y = self->oam[i * 4];

        /* Y >= 239 is offscreen */
        if (y >= 239 || self->scanline < y || self->scanline >= y + h) {
            continue;
        }

        /* Sprite Y is always offset by +1 */
        unsigned row = self->scanline - y;

        if (row >= 0 && row < h) {
            Sprite *spr = &self->queued_sprites[self->queued_sprites_count];

            uint8_t tile = self->oam[i * 4 + 1];
            uint8_t attr = self->oam[i * 4 + 2];
            uint16_t pat_addr;

            spr->pos_y = y;
            spr->pos_x = self->oam[i * 4 + 3];
            spr->attr = attr;
            spr->is_0 = (i == 0);

            /* Precalculate the vertical flip fine-y */
            unsigned y_fine = (attr & 0x80) ? (h - 1 - row) : row;

            /* Precalculate the CHR address */
            if (h == 8) {
                /* 8x8 mode */
                uint16_t base = get_pattern_table_spr(self);
                pat_addr = base + (tile * 16) + (uint16_t) y_fine;
            } else {
                /* 8x16 mode: bit 0 selects the bank */

                uint16_t base = (tile & 1) ? 0x1000 : 0x0000;
                uint8_t tile_idx = tile & 0xfe;

                if (y_fine >= 8) {
                    ++tile_idx;
                    y_fine -= 8;
                }

                pat_addr = base + (tile_idx * 16) + (uint16_t) y_fine;
            }

            spr->tile = fetch_spr_tile(self, pat_addr, attr);

            ++self->queued_sprites_count;
        }
    }

    check_sprite_overflow(self, i, h);
}

static void NOTFLASH_FN(render_bg)(Ppu *self, unsigned start, unsigned end) {
    unsigned curr = start;
    unsigned offset = (curr + self->x) & 7;
    uint8_t *dest = &self->scanline_buf[curr];

    while (curr < end) {
        /* pixels remaining in the tile */
        unsigned rem = 8 - offset;
        unsigned count = (rem > (end - curr)) ? (end - curr) : rem;
        TileRow tile = fetch_tile(self);

        tile >>= (offset << 2);

        for (unsigned i = 0; i < count; ++i) {
            *dest++ = tile & 0x0f;

            tile >>= 4;
            ++curr;
        }

        offset = 0;

        if (!((curr + self->x) & 7)) {
            increment_coarse_x(self);
        }
    }
}

static void
NOTFLASH_FN(render_sprites)(Ppu *self, unsigned start, unsigned end) {
    for (int i = (int) self->queued_sprites_count - 1; i >= 0; --i) {
        Sprite *spr = &self->queued_sprites[i];

        unsigned spr_end = spr->pos_x + 8;
        unsigned draw_start = (spr->pos_x > start) ? spr->pos_x : start;
        unsigned draw_end = (spr_end < end) ? spr_end : end;

        if (draw_start >= draw_end) {
            continue;
        }

        if (draw_start > spr->pos_x) {
            spr->tile >>= ((draw_start - spr->pos_x) << 2);
        }

        TileRow tile = spr->tile;
        bool behind_bg = (spr->attr & 0x20);
        uint8_t *dest = &self->scanline_buf[draw_start];

        for (unsigned x = draw_start; x < draw_end; ++x) {
            uint8_t px = tile & 0x0f;
            uint8_t bg_px = *dest & 0x0f;
            bool bg_opaque = (bg_px & 0x03) != 0;

            if ((px & 0x03) != 0) {
                if (spr->is_0 && bg_opaque && x < 255) {
                    set_spr_0_hit(self, true);
                }

                if (!behind_bg || !bg_opaque) {
                    *dest = px | 0x10;
                }
            }

            ++dest;
            ++bg_opaque;
            tile >>= 4;
        }
    }
}

void NOTFLASH_FN(ppu_sync)(Ppu *self) {
    if (self->scanline < 240 && self->last_rendered_dot < 256) {
        unsigned curr_dot = (self->dot < 256) ? 256 : self->dot;

        if (curr_dot > self->last_rendered_dot) {
            if (self->mask & PPU_MASK_BG_ON) {
                render_bg(self, self->last_rendered_dot, curr_dot);
            } else {
                memset(
                    self->scanline_buf + self->last_rendered_dot, 0, curr_dot
                );
            }

            if ((self->mask & PPU_MASK_SPR_ON) &&
                self->queued_sprites_count > 0) {
                render_sprites(self, self->last_rendered_dot, curr_dot);
            }

            self->last_rendered_dot = curr_dot;
        }
    }
}

void NOTFLASH_FN(ppu_run_until)(Ppu *self, CycleCounter target_cycle) {
    bool bg_on = self->mask & PPU_MASK_BG_ON;
    bool spr_on = self->mask & PPU_MASK_SPR_ON;
    bool render_on = bg_on || spr_on;

    while ((CycleDiff) (target_cycle - self->cycles) > 0) {
        unsigned cycles_rem = SCANLINE_LENGTH - self->dot;
        unsigned cycles_to_run = (unsigned) (target_cycle - self->cycles);

        unsigned step =
            (cycles_to_run < cycles_rem) ? cycles_to_run : cycles_rem;

        unsigned prev_dot = self->dot;
        unsigned next_dot = self->dot + step;

        if (prev_dot < 260 && next_dot >= 260 && self->scanline < 240 &&
            render_on) {
            bus_hsync(self->bus);
        }

        if (prev_dot < 256 && next_dot >= 256) {
            ppu_sync(self);

            if ((self->scanline < 240 || self->scanline == 261) && render_on) {
                increment_coarse_y(self);
            }

            if (self->scanline < 240 && self->scanline_cb) {
                self->scanline_cb(self);
            }
        }

        if (render_on) {
            if (prev_dot < 257 && next_dot >= 257 &&
                (self->scanline < 240 || self->scanline == 261)) {
                self->v = (self->v & ~0x041F) | (self->t & 0x041F);

                queue_sprites(self);
            }

            if (self->scanline == 261 && prev_dot < 280 && next_dot >= 280) {
                self->v = (self->v & ~0x7BE0) | (self->t & 0x7BE0);
            }
        }

        self->cycles += step;
        self->dot = next_dot;

        if (self->dot >= 341) {
            ++self->scanline;
            self->dot -= 341;
            self->last_rendered_dot = 0;

            if (self->scanline == 241) {
                set_vblank(self, true);
                trigger_nmi(self);
            } else if (self->scanline > 261) {
                self->scanline = 0;
                self->frame_done = true;

                set_vblank(self, false);
                set_spr_0_hit(self, false);
                self->status &= ~PPU_STAT_OVERFLOW;
            }
        }
    }
}

/*
 * ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
 * ░░░░░░░░░░░░░░▄▄▀▀▀▀▀▀▀▀▄▄░░░░░░░░░░░░░░
 * ░░░░░░░░░▄██▄▀░░░░░░░░░░░░▀▄██▄░░░░░░░░░
 * ░░░░░░░░░░░███░░░░░░░░░░░░███░░░░░░░░░░░
 * ░░░░░░░░░░█░▀██░░░░░░░░░░██▀░█░░░░░░░░░░
 * ░░░░░░░░░█░░░▄██▄░░░░░░▄██▄░░░█░░░░░░░░░
 * ░░░░░░░░▄▀░▄▀──▀█▄░░░░▄█▀──▀▄░▀▄░░░░░░░░
 * ░░░░░░░▄▀░▄▀───▄─██░░██─▄───▀▄░▀▄░░░░░░░
 * ░░░░░▄▀░░░█───███─█░░█─███───█░░░▀▄░░░░░
 * ░░░▄▀░░░░░█────▀──█░░█──▀────█░░░░░▀▄░░░
 * ░▄▀░░░░░░░█──────█░░░░█──────█░░░░░░░▀▄░
 * █░░░░░▄░░░░▀▄▄▄▄▀░░░░░░▀▄▄▄▄▀░░░░▄░░░░░█
 * █░░░░░▌▀▄░░░░░░░░░░░░░░░░░░░░░░▄▀▐░░░░░█
 * █░░░░▐▄▄▄█▄▄▄▄▄▀▀▀▀▀▀▀▀▀▀▄▄▄▄▄█▄▄▄▌░░░░█
 * █░░░░▀░░░░░░░░░░░░░░░░░░░░░░░░░░░░▀░░░░█
 * ░▀▄░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░▄▀░
 * ░░░▀▀▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▄▀▀░░░
 * ░░░░░░░░░░░█░░░░░░░░░░░░░░░░█░░░░░░░░░░░
 * ░░░░░░░░░░░█░░░░░░░░░░░░░░░░█░░░░░░░░░░░
 * ░░░░░░░░░░░█░░░░░░░░░░░░░░░░█░░░░░░░░░░░
 * ░░░░░░░░░░░█░░░░░░░░░░░░░░░░█░░░░░░░░░░░
 * ░░░░░░░░░░░█░░░░░░░░░░░░░░░░█░░░░░░░░░░░
 * ░░░░░░░▄▄▀▀▀▀▄░░░░░░░░░░░░▄▀▀▀▀▄▄░░░░░░░
 * ░░░░░▄▀░░░░░░░▀▄▄▄▄▄▄▄▄▄▄▀░░░░░░░▀▄░░░░░
 * ░░░░░█░░░░░░░░░░░▄▀░░▀▄░░░░░░░░░░░█░░░░░
 * ░░░░░░▀▄░░░░░░░▄▀░░░░░░▀▄░░░░░░░▄▀░░░░░░
 * ░░░░░░░░▀▀▀▀▀▀▀░░░░░░░░░░▀▀▀▀▀▀▀░░░░░░░░
 * ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
 * ░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░░
 */
