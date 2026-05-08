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
 * @file ppu.h
 * @brief NES Picture Processing Unit (PPU) emulation.
 *
 * The PPU is the graphics chip of the NES. It is responsible for drawing
 * the background (made of tiles) and up to 64 sprites onto a 256x240 pixel
 * screen at ~60 frames per second.
 *
 * The PPU runs independently of the CPU. Both chips share a common bus, but
 * the PPU has its own separate address space (PPU bus) which holds:
 *   - Pattern tables (CHR ROM/RAM) – raw tile pixel data
 *   - Nametables (VRAM)            – which tile to draw at each screen position
 *   - Attribute tables             – which colour palette to use per tile block
 *   - Palette RAM                  – 32 bytes of colour index entries
 *
 * The PPU works by scanning through the screen line by line (scanline by
 * scanline). Each scanline takes exactly 341 PPU clock cycles (called "dots").
 * A full frame takes 262 scanlines.
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

/* =========================================================================
 * PPU CTRL register ($2000) – write-only
 *
 * Controls general PPU behaviour. Written by the CPU to configure rendering.
 * ========================================================================= */

/** @defgroup ppu_ctrl PPU Control Register ($2000) flags
 *  @{
 */

/** Bits 0-1: Select the base nametable address.
 *  0 = $2000, 1 = $2400, 2 = $2800, 3 = $2C00. */
#define PPU_CTRL_NAMETABLE_ADDR 0x3

/** Bit 2: VRAM address increment per CPU read/write of $2007.
 *  0 = increment by 1 (move right one tile column),
 *  1 = increment by 32 (move down one tile row). */
#define PPU_CTRL_INC_MODE (1u << 2)

/** Bit 3: Sprite pattern table address for 8x8 sprites.
 *  0 = $0000 (pattern table 0), 1 = $1000 (pattern table 1).
 *  Ignored when 8x16 sprite mode is active (PPU_CTRL_SPR_HEIGHT). */
#define PPU_CTRL_SPR_TILE_SELECT (1u << 3)

/** Bit 4: Background pattern table address.
 *  0 = $0000 (pattern table 0), 1 = $1000 (pattern table 1). */
#define PPU_CTRL_BG_TILE_SELECT (1u << 4)

/** Bit 5: Sprite size.
 *  0 = 8x8 pixels, 1 = 8x16 pixels. */
#define PPU_CTRL_SPR_HEIGHT (1u << 5)

/** Bit 6: PPU master/slave select. Rarely used. */
#define PPU_CTRL_MS (1u << 6)

/** Bit 7: Generate a Non-Maskable Interrupt (NMI) at the start of VBlank.
 *  When set, the CPU is interrupted every frame so it can update graphics. */
#define PPU_CTRL_NMI_ON (1u << 7)

/** @} */

/* =========================================================================
 * PPU MASK register ($2001) – write-only
 *
 * Controls what is shown on screen and applies colour effects.
 * ========================================================================= */

/** @defgroup ppu_mask PPU Mask Register ($2001) flags
 *  @{
 */

/** Bit 0: Greyscale mode. When set, all colours are converted to greyscale. */
#define PPU_MASK_GREYSCALE (1u << 0)

/** Bit 1: Show background in the leftmost 8 pixels of the screen.
 *  When clear, the left edge is blank (useful to hide artefacts during scroll). */
#define PPU_MASK_BG_LEFT_COL_ON (1u << 1)

/** Bit 2: Show sprites in the leftmost 8 pixels of the screen. */
#define PPU_MASK_SPR_LEFT_COL_ON (1u << 2)

/** Bit 3: Enable background rendering. When clear, no background is drawn. */
#define PPU_MASK_BG_ON (1u << 3)

/** Bit 4: Enable sprite rendering. When clear, no sprites are drawn. */
#define PPU_MASK_SPR_ON (1u << 4)

/** Bit 5: Emphasise red channel (shifts the colour balance toward red). */
#define PPU_MASK_EMPH_RED (1u << 5)

/** Bit 6: Emphasise green channel. */
#define PPU_MASK_EMPH_GREEN (1u << 6)

/** Bit 7: Emphasise blue channel. */
#define PPU_MASK_EMPH_BLUE (1u << 7)

/** @} */

/* =========================================================================
 * PPU STATUS register ($2002) – read-only
 *
 * Reports current PPU state. Reading this register also clears the VBlank
 * flag and resets the address latch (w).
 * ========================================================================= */

/** @defgroup ppu_status PPU Status Register ($2002) flags
 *  @{
 */

/** Bit 5: Sprite overflow flag.
 *  Set when more than 8 sprites appear on the same scanline.
 *  (The real hardware has a bug in how this is detected.) */
#define PPU_STAT_OVERFLOW (1u << 5)

/** Bit 6: Sprite 0 hit flag.
 *  Set when a non-transparent pixel of sprite 0 overlaps a non-transparent
 *  background pixel. Games use this to split the screen (e.g. status bars). */
#define PPU_STAT_ZERO_HIT (1u << 6)

/** Bit 7: Vertical Blank (VBlank) flag.
 *  Set at the start of VBlank (scanline 241) and cleared at the end of it.
 *  VBlank is the safe period when the CPU can update VRAM without corrupting
 *  the image currently being drawn. */
#define PPU_STAT_VBLANK (1u << 7)

/** @} */

/* =========================================================================
 * Screen dimensions
 * ========================================================================= */

/** Width of the rendered picture in pixels. */
#define PPU_WIDTH 256

/** Height of the rendered picture in pixels. */
#define PPU_HEIGHT 240

/* =========================================================================
 * CPU-side PPU register addresses (on the main CPU bus, $2000-$2007)
 * ========================================================================= */

/** @defgroup ppu_cpu_regs CPU-visible PPU register addresses
 *  @{
 */
#define PPU_CTRL   0x2000 /**< Control register (W). */
#define PPU_MASK   0x2001 /**< Mask register (W). */
#define PPU_STAT   0x2002 /**< Status register (R). */
#define PPU_SCROLL 0x2005 /**< Scroll position (W, write twice: X then Y). */
#define PPU_ADDR   0x2006 /**< VRAM address (W, write twice: high byte then low byte). */
#define PPU_DATA   0x2007 /**< VRAM data (R/W). Each access auto-increments the address. */
/** @} */

/* =========================================================================
 * OAM (Object Attribute Memory) register addresses
 *
 * OAM stores the position, tile index, and attributes of all 64 sprites.
 * Each sprite occupies 4 bytes, giving 256 bytes total.
 * ========================================================================= */

/** @defgroup oam_regs OAM register addresses
 *  @{
 */
#define OAM_ADDR 0x2003 /**< Set the byte offset within OAM to read/write (W). */
#define OAM_DATA 0x2004 /**< Read/write a single byte at the current OAM address (R/W). */
#define OAM_DMA  0x4014 /**< Trigger a 256-byte DMA copy from CPU RAM page into OAM (W).
                          *   This is the fast, standard way to upload sprite data each frame. */
/** @} */

/* =========================================================================
 * PPU address space layout (on the PPU bus, 0x0000-0x3FFF)
 * ========================================================================= */

/** @defgroup ppu_addr_space PPU address space constants
 *  @{
 */

/** Pattern table 0 base address ($0000-$0FFF). Holds tile pixel data (usually CHR ROM). */
#define PPU_PT0 0x0000

/** Pattern table 1 base address ($1000-$1FFF). Second bank of tile pixel data. */
#define PPU_PT1 0x1000

/** Nametable 0 base address ($2000). Stores the tile map for the top-left screen quadrant. */
#define PPU_NT0 0x2000

/** Attribute table 0 ($23C0). Stores palette assignments for 2x2 tile blocks in nametable 0. */
#define PPU_AT0 0x23c0

/** Nametable 1 base address ($2400). Top-right screen quadrant. */
#define PPU_NT1 0x2400

/** Attribute table 1 ($27C0). */
#define PPU_AT1 0x27c0

/** Nametable 2 base address ($2800). Bottom-left screen quadrant. */
#define PPU_NT2 0x2800

/** Attribute table 2 ($2BC0). */
#define PPU_AT2 0x2bc0

/** Nametable 3 base address ($2C00). Bottom-right screen quadrant. */
#define PPU_NT3 0x2c00

/** Attribute table 3 ($2FC0). */
#define PPU_AT3 0x2fc0

/**
 * @defgroup ppu_palettes Palette RAM addresses ($3F00-$3F1F)
 *
 * The NES has 8 palettes of 4 colours each (32 bytes total).
 * The first 4 are used by the background, the last 4 by sprites.
 * Colour index 0 of every palette is the universal backdrop colour.
 * @{
 */
#define PPU_BG_PALLETE0  0x3f00 /**< Background palette 0. */
#define PPU_BG_PALLETE1  0x3f04 /**< Background palette 1. */
#define PPU_BG_PALLETE2  0x3f08 /**< Background palette 2. */
#define PPU_BG_PALLETE3  0x3f0c /**< Background palette 3. */
#define PPU_SPR_PALLETE0 0x3f10 /**< Sprite palette 0. */
#define PPU_SPR_PALLETE1 0x3f14 /**< Sprite palette 1. */
#define PPU_SPR_PALLETE2 0x3f18 /**< Sprite palette 2. */
#define PPU_SPR_PALLETE3 0x3f1c /**< Sprite palette 3. */
/** @} */

/** @} */ /* end ppu_addr_space */

/* =========================================================================
 * Internal VRAM address register (v / t) bit-field masks
 *
 * Both v (current VRAM address) and t (temporary VRAM address) use a 15-bit
 * layout that encodes scroll position and nametable selection in one value.
 *
 *  Bit layout:  yyy NN YYYYY XXXXX
 *               |   || |     |
 *               |   || |     +-- Coarse X (0-31): current tile column
 *               |   || +-------- Coarse Y (0-29): current tile row
 *               |   |+---------- Nametable X select (horizontal)
 *               |   +----------- Nametable Y select (vertical)
 *               +--------------- Fine Y (0-7): pixel row within the current tile
 * ========================================================================= */

/** @defgroup vram_addr_fields VRAM address register bit-field masks and accessors
 *  @{
 */

/** Bits 0-4: Coarse X scroll – which tile column (0-31) is currently being fetched. */
#define VRAM_COARSE_X  0x1f

/** Bits 5-9: Coarse Y scroll – which tile row (0-31) is currently being fetched. */
#define VRAM_COARSE_Y  0x3e0

/** Bits 10-11: Nametable select – which of the 4 nametables is active. */
#define VRAM_NT_SEL    0xc00

/** Bit 10: Horizontal nametable select (selects left or right nametable). */
#define VRAM_NTX_SEL   0x400

/** Bit 11: Vertical nametable select (selects top or bottom nametable). */
#define VRAM_NTY_SEL   0x800

/** Bits 12-14: Fine Y scroll – which pixel row (0-7) within the current tile is being drawn. */
#define VRAM_FINE_Y    0x7000

/** Extract the coarse X field from a VRAM address value @p v. */
#define vram_get_coarse_x(v)   ((v) & VRAM_COARSE_X)

/** Extract the coarse Y field from a VRAM address value @p v. */
#define vram_get_coarse_y(v)   (((v) & VRAM_COARSE_Y) >> 5)

/** Extract the nametable selection bits (0-3) from a VRAM address value @p v. */
#define vram_get_nametable(v)  (((v) & VRAM_NT_SEL) >> 10)

/** Extract the fine Y field (0-7) from a VRAM address value @p v. */
#define vram_get_fine_y(v)     (((v) & VRAM_FINE_Y) >> 12)

/** @} */

/*
 * V is incremented during rendering:
 *   - Coarse X: incremented on every tile fetch (every 8 dots).
 *   - Fine Y / coarse Y: incremented at the end of each scanline.
 *
 * T & V relationship:
 *   - At specific dots (257, 280-304) during rendering, the scroll position
 *     stored in T is copied back into V so each new scanline starts correctly.
 *
 * X (fine X scroll):
 *   - A 3-bit register separate from V, used to shift fetched tile pixels
 *     horizontally within the 8-pixel tile window.
 */

typedef struct Bus Bus;
typedef struct Ppu Ppu;

/**
 * @brief Callback invoked by the PPU at the end of each rendered scanline.
 *
 * The host application (or platform layer) provides this callback to consume
 * the rendered pixel row (stored in @ref Ppu::scanline_buf) immediately after
 * it is produced, e.g. to blit it to a display or framebuffer.
 *
 * @param self Pointer to the PPU instance that finished the scanline.
 */
typedef void (*PpuScanlineCallback)(Ppu *self);

/**
 * @brief Identifies the type (purpose) of the current scanline.
 *
 * A full NES frame consists of 262 scanlines total:
 *   - 1  pre-render scanline (dummy scanline, prepares state for the next frame)
 *   - 240 visible scanlines  (pixels are actually drawn)
 *   - 1  post-render scanline (idle)
 *   - 20 VBlank scanlines    (CPU can safely update VRAM here)
 */
typedef enum ScanlineId {
    SCANLINE_PRE_RENDER,  /**< Scanline -1 / 261: dummy scanline, resets PPU state. */
    SCANLINE_VISIBLE,     /**< Scanlines 0-239: active picture area, pixels are rendered. */
    SCANLINE_VBLANK,      /**< Scanlines 241-260: vertical blank, CPU updates VRAM here. */
    SCANLINE_POST_RENDER  /**< Scanline 240: PPU is idle, no rendering occurs. */
} ScanlineId;

/**
 * @brief Holds a single row of pixel colour indices for one 8-pixel-wide tile.
 *
 * Each byte in the 32-bit value encodes one pixel's 2-bit palette index
 * (packed as 4 bytes for convenient shifting during rendering).
 */
typedef uint32_t TileRow;

/**
 * @brief Output pixel type.
 *
 * On RP2350 targets a 16-bit RGB565 value is used to save memory bandwidth.
 * On all other platforms a 32-bit ARGB/XRGB value is used.
 */
#ifdef BUILD_RP2350
typedef uint16_t PpuPixel;
#else
typedef uint32_t PpuPixel;
#endif

/**
 * @brief Represents one sprite that is queued to be drawn on the current scanline.
 *
 * The PPU can display up to 8 sprites per scanline. During the sprite
 * evaluation phase (dots 65-256 of each scanline) the PPU scans OAM and
 * selects up to 8 sprites whose Y range covers the next scanline.
 */
typedef struct Sprite {
    uint8_t pos_y;  /**< Y position of the sprite on screen (top pixel row). */
    TileRow tile;   /**< Pre-fetched pixel row data for this scanline. */
    uint8_t attr;   /**< Sprite attribute byte: palette, priority, flip flags. */
    uint8_t pos_x;  /**< X position of the sprite on screen (leftmost pixel). */
    bool is_0;      /**< True if this sprite is sprite #0 (used for zero-hit detection). */
} Sprite;

/**
 * @brief Main PPU state structure.
 *
 * This struct holds the complete state of the PPU, including all internal
 * registers, memory, and rendering state. One instance exists per emulated
 * NES system.
 */
typedef struct Ppu {
    Bus *bus; /**< Pointer to the shared system bus (used for CPU-PPU communication). */

    /* -----------------------------------------------------------------
     * CPU-visible registers ($2000-$2007 / $4014)
     * ----------------------------------------------------------------- */

    uint8_t  ctrl;     /**< PPUCTRL ($2000): controls NMI, pattern table selection, etc. */
    uint8_t  mask;     /**< PPUMASK ($2001): enables/disables rendering and colour effects. */
    uint8_t  status;   /**< PPUSTATUS ($2002): VBlank, sprite-0 hit, and overflow flags. */
    uint16_t scroll;   /**< PPUSCROLL ($2005): background scroll position (internal copy). */
    uint8_t  data;     /**< PPUDATA ($2007): last value written/read via the data port. */
    uint8_t  oam_addr; /**< OAMADDR ($2003): byte index into OAM for the next OAM access. */

    /* -----------------------------------------------------------------
     * Internal scroll / address registers (Loopy registers)
     *
     * These implement the well-documented "Loopy" scroll mechanism.
     * ----------------------------------------------------------------- */

    uint16_t v; /**< Current VRAM address (15 bits). Also encodes current scroll position
                  *   during rendering. Incremented by the PPU as it fetches tile data. */
    uint16_t t; /**< Temporary VRAM address (15 bits). Holds the scroll position written
                  *   by the CPU. Copied to v at the start of each new line during rendering. */
    uint8_t  x; /**< Fine X scroll (3 bits). Sub-tile horizontal offset (0-7). */
    uint8_t  w; /**< Write toggle (1 bit). Tracks whether the next write to PPUADDR /
                  *   PPUSCROLL is the first (0) or second (1) byte of the two-write sequence. */

    /* -----------------------------------------------------------------
     * Per-scanline sprite state
     * ----------------------------------------------------------------- */

    Sprite   queued_sprites[8];      /**< Up to 8 sprites selected for the current scanline. */
    unsigned queued_sprites_count;   /**< How many sprites are in queued_sprites (0-8). */

    /* -----------------------------------------------------------------
     * Rendering output
     * ----------------------------------------------------------------- */

    /** Pixel colour index buffer for the current scanline (one byte per pixel).
     *  Written during rendering; consumed by @ref scanline_cb. */
    uint8_t scanline_buf[PPU_WIDTH];

    /** Callback called at the end of each scanline so the host can display the pixels. */
    PpuScanlineCallback scanline_cb;

    /** Arbitrary pointer passed through to @ref scanline_cb for host-side context. */
    void *user_data;

    /** Pointer to the full 64-entry NES master palette, as host pixel values. */
    const PpuPixel *colors;

    /** Look-up table mapping the 32 palette RAM entries to host pixel values.
     *  Rebuilt whenever palette RAM changes to avoid per-pixel palette lookups. */
    PpuPixel colors_lut[32];

    /** When true the colors_lut must be regenerated before the next frame (RP2350 only). */
    bool colors_lut_dirty;

    /* -----------------------------------------------------------------
     * PPU memory
     * ----------------------------------------------------------------- */

    uint8_t  vram[0x800];  /**< Internal VRAM (2 KB). Stores the two active nametables
                             *   (the other two are either mirrored or from cartridge RAM). */
    uint8_t  oam[0x100];   /**< Object Attribute Memory (256 bytes = 64 sprites × 4 bytes). */
    uint8_t  palettes[32]; /**< Palette RAM (32 bytes = 8 palettes × 4 colour indices). */
    uint8_t *chr;          /**< Pointer to CHR data (pattern tables). Points into cartridge
                             *   ROM or RAM depending on the mapper. */

    uint8_t temp_read_buf; /**< Read buffer for $2007 reads. The NES PPU returns data from
                             *   the *previous* read when reading $2007 (one-cycle delay). */

    Mirroring mirroring;   /**< Current nametable mirroring mode (horizontal, vertical, etc.).
                             *   Determines how the 2 KB VRAM is mapped to 4 logical nametables. */

    /* -----------------------------------------------------------------
     * Timing state
     * ----------------------------------------------------------------- */

    unsigned     scanline;           /**< Current scanline being processed (0-261). */
    unsigned     dot;                /**< Current dot (pixel clock) within the scanline (0-340). */
    CycleCounter cycles;             /**< Total PPU cycles elapsed since reset. */
    unsigned     last_rendered_dot;  /**< Dot at which the last render step ended (for catch-up). */
    bool         frame_done;         /**< Set to true when the PPU finishes a complete frame. */

    TileRow cached_tile; /**< Tile row fetched in the current 8-dot tile fetch cycle,
                           *   shifted out one pixel at a time during rendering. */

    uint8_t *nt[4]; /**< Pointers to the four logical nametables (NT0-NT3).
                      *   Each pointer resolves to either a region of vram[] or
                      *   cartridge-side RAM, according to the current mirroring mode. */
} Ppu;

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Read a PPU register from the CPU bus (has side-effects).
 *
 * Handles the NES register read side-effects such as clearing the VBlank flag
 * when $2002 is read, or returning data from the internal read buffer for $2007.
 *
 * @param self Pointer to the PPU instance.
 * @param addr CPU bus address of the register ($2000-$2007).
 * @return The byte value returned to the CPU.
 */
uint8_t ppu_read_reg(Ppu *self, uint16_t addr);

/**
 * @brief Read a PPU register without triggering side-effects (for debugging).
 *
 * Useful for debuggers or savestates that need to inspect register values
 * without disturbing emulation state.
 *
 * @param self Pointer to the PPU instance (const – no state is modified).
 * @param reg  CPU bus address of the register.
 * @return The current raw value of the register.
 */
uint8_t ppu_peek_reg(const Ppu *self, uint16_t reg);

/**
 * @brief Write a value to a CPU-visible PPU register (has side-effects).
 *
 * Handles all write side-effects such as updating the t/v/x/w scroll
 * registers, triggering OAM DMA, etc.
 *
 * @param self Pointer to the PPU instance.
 * @param addr CPU bus address of the register ($2000-$2007, $4014).
 * @param data The byte to write.
 */
void ppu_write_reg(Ppu *self, uint16_t addr, uint8_t data);

/**
 * @brief Write a byte directly to the PPU address space.
 *
 * Routes the write to the correct destination: CHR RAM, VRAM (nametable),
 * or palette RAM, based on @p addr.
 *
 * @param self Pointer to the PPU instance.
 * @param addr PPU bus address (0x0000-0x3FFF).
 * @param data The byte to write.
 */
void ppu_write(Ppu *self, uint16_t addr, uint8_t data);

/**
 * @brief Read a byte directly from the PPU address space.
 *
 * Routes the read to CHR, VRAM, or palette RAM.
 *
 * @param self Pointer to the PPU instance.
 * @param addr PPU bus address (0x0000-0x3FFF).
 * @return The byte at that address.
 */
uint8_t ppu_read(Ppu *self, uint16_t addr);

/**
 * @brief Write one byte into OAM at the current oam_addr offset.
 *
 * Advances oam_addr by 1 after the write (wraps at 256).
 * Used during OAM DMA and direct $2004 writes.
 *
 * @param self Pointer to the PPU instance.
 * @param data The byte to store in OAM.
 */
void ppu_oam_write(Ppu *self, uint8_t data);

/**
 * @brief Initialise a Ppu instance and link it to the system bus.
 *
 * Must be called once before any other ppu_* function. Sets all registers
 * and memory to their power-on state.
 *
 * @param self Pointer to the Ppu struct to initialise.
 * @param bus  Pointer to the system bus this PPU is connected to.
 */
void ppu_init(Ppu *self, Bus *bus);

/**
 * @brief Build the colour look-up table (colors_lut) from the current palette RAM.
 *
 * Translates the 32 palette RAM entries through the master NES colour table
 * into host pixel format. Called after palette RAM changes or after the master
 * colour table pointer is updated.
 *
 * @param self Pointer to the PPU instance.
 */
void ppu_init_pixel_luts(Ppu *self);

/**
 * @brief Reset the PPU to its post-reset state.
 *
 * Clears control/mask/status registers and resets the scroll/address latches.
 * Does not clear VRAM or OAM (matching real hardware behaviour).
 *
 * @param self Pointer to the PPU instance.
 */
void ppu_reset(Ppu *self);

/**
 * @brief Catch-up rendering: run the PPU up to the current CPU cycle count.
 *
 * The PPU and CPU run at different clock rates (3 PPU cycles per CPU cycle on
 * NTSC). This function is called before the CPU accesses any PPU register so
 * that the PPU state is up-to-date.
 *
 * @param self Pointer to the PPU instance.
 */
void ppu_sync(Ppu *self);

/**
 * @brief Run the PPU until its cycle counter reaches @p target_cycle.
 *
 * Processes dots and scanlines in order, calling @ref ppu_render_scanline and
 * @ref ppu_scanline_end as needed. The main rendering loop entry point.
 *
 * @param self         Pointer to the PPU instance.
 * @param target_cycle PPU cycle count to advance to.
 */
void ppu_run_until(Ppu *self, CycleCounter target_cycle);

/**
 * @brief Perform end-of-scanline bookkeeping.
 *
 * Updates the VRAM address register v (coarse/fine Y increment, nametable
 * switch), fires the scanline callback, and handles VBlank entry/exit and
 * NMI generation.
 *
 * @param self Pointer to the PPU instance.
 */
void ppu_scanline_end(Ppu *self);

/**
 * @brief Render all pixels of the current scanline into scanline_buf.
 *
 * Fetches background tile data and evaluates sprites, then blends them
 * according to priority rules. The result (one palette index per pixel) is
 * written to @ref Ppu::scanline_buf.
 *
 * @param self Pointer to the PPU instance.
 */
void ppu_render_scanline(Ppu *self);

#ifdef __cplusplus
}
#endif
