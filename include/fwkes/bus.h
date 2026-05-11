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
 * @file bus.h
 * @brief System bus – the central coordinator of the NES emulator.
 *
 * In the real NES the bus is the set of electrical wires connecting the CPU,
 * PPU, APU, cartridge, and controllers.  In this emulator the Bus struct plays
 * the same role: it owns every component and is the single point through which
 * they communicate.
 *
 * ## Responsibilities
 *
 * **Memory routing** – bus_read() and bus_write() implement the NES CPU memory
 * map (table 2.2 in the paper).  Instead of a chain of if-else comparisons,
 * the address is divided into 4 KiB pages with a switch statement so the
 * compiler can generate an efficient jump table.
 *
 * **Catch-up synchronisation** – the NES CPU, PPU, and APU all run at different
 * clock rates and in parallel on real hardware.  Here they run sequentially.
 * Whenever the CPU touches a PPU register, bus_write() / bus_read() first calls
 * ppu_run_until() so the PPU catches up to the current CPU cycle count before
 * the register access takes effect.  This is the "catch-up on demand" mechanism
 * described in section 1.3 of the paper.
 *
 * **Event queue** – some operations (loading a new ROM, resetting the system)
 * are requested from within the emulated program via the FWX interface but must
 * not be executed immediately, as that would corrupt the emulator state mid-
 * instruction.  They are placed on a BusQueue and processed at a safe point
 * between frames by bus_update().
 *
 * **Component ownership** – the Bus struct contains every subsystem by value
 * (Cpu, Ppu, Apu, Disk, Joypad x2, Fwx).  This keeps them all in one
 * contiguous memory block, which is important on the RP2350 where SRAM is only
 * 520 KB.
 */

#pragma once

#include "apu.h"
#include "cpu.h"
#include "joypad.h"
#include "disk.h"
#include "fs.h"
#include "fwx/private.h"
#include "ppu.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Memory size constants
 * ========================================================================= */

/** Total CPU address space size in bytes (2^16 = 65 536).
 *  The 6502 has a 16-bit address bus so it can address exactly this much. */
#define MEMORY_SIZE 0x10000

/** Size of the zero page in bytes (256 bytes, addresses $0000-$00FF).
 *  The zero page gets its own faster addressing modes in the 6502 instruction
 *  set, so programs keep their most-used variables here. */
#define ZEROPAGE_SIZE 0x100

/** Maximum number of events that can be queued at once in a BusQueue. */
#define BUS_EVENT_QUEUE_CAP 32

/* =========================================================================
 * Event system
 *
 * The event queue decouples "when a request arrives" from "when it is safe
 * to act on it".  Emulated code (e.g. the BIOS via FWX) can post an event
 * at any point during emulation; the host firmware then drains the queue
 * between frames via bus_update(), at which point the emulator state is
 * in a known, consistent condition.
 * ========================================================================= */

/**
 * @brief Identifies which type of event is stored in a BusEvent.
 */
typedef enum BusEventId {
    BUS_EVENT_NONE,         /**< Placeholder / empty slot (no action needed). */
    BUS_EVENT_LOAD_ROM,     /**< Load a ROM file from the filesystem and start running it. */
    BUS_EVENT_SET_FWX_ERR,  /**< Set the FWX error code in the FWXSTAT register ($4018). */
    BUS_EVENT_RESET         /**< Perform a full system reset (equivalent to pressing Reset). */
} BusEventId;

/**
 * @brief Payload for a BUS_EVENT_LOAD_ROM event.
 *
 * @warning The @p path pointer must remain valid until bus_update() processes
 *          this event.  The Bus does not copy the string.
 */
typedef struct BusEventLoadRom {
    const char *path; /**< Null-terminated path to the .nes ROM file on the SD card. */
} BusEventLoadRom;

/**
 * @brief Payload for a BUS_EVENT_SET_FWX_ERR event.
 *
 * Used by FWX command handlers to report an error code back to the emulated
 * program.  The code is written into the ER0-ER3 bits of the FWXSTAT register
 * ($4018) so the BIOS can read it after a command completes.
 */
typedef struct BusSetFwxError {
    FwxError err; /**< The FWX error code to store in FWXSTAT. */
} BusSetFwxError;

/**
 * @brief A single item in the bus event queue.
 *
 * The active payload field is determined by @ref BusEvent::id.  Only the union
 * member that matches the id is valid.
 */
typedef struct BusEvent {
    BusEventId id; /**< Which event this is (determines which union member is valid). */

    union {
        BusEventLoadRom load_rom;    /**< Valid when id == BUS_EVENT_LOAD_ROM. */
        BusSetFwxError  set_fwx_err; /**< Valid when id == BUS_EVENT_SET_FWX_ERR. */
    };
} BusEvent;

/**
 * @brief Circular FIFO queue of pending bus events.
 *
 * Events are added at the tail and consumed from the head.  The capacity is
 * fixed at BUS_EVENT_QUEUE_CAP; adding more events than that will overwrite
 * the oldest unprocessed entry (overflow should not happen in normal use).
 */
typedef struct BusQueue {
    BusEvent events[BUS_EVENT_QUEUE_CAP]; /**< Fixed-size ring buffer of events. */
    unsigned count;                        /**< Number of events currently in the queue. */
    unsigned head;                         /**< Index of the next event to be consumed. */
    unsigned tail;                         /**< Index where the next event will be written. */
} BusQueue;

/**
 * @brief Initialise an empty BusQueue.
 * @param self The queue to initialise.
 */
void bus_queue_init(BusQueue *self);

/**
 * @brief Discard all events currently in the queue.
 * @param self The queue to clear.
 */
void bus_queue_clear(BusQueue *self);

/**
 * @brief Append an event to the back of the queue.
 *
 * The event struct is copied into the ring buffer, so the caller does not need
 * to keep the original alive (except for pointer fields inside the event
 * payload – see BusEventLoadRom).
 *
 * @param self The queue to push onto.
 * @param ev   Pointer to the event to enqueue.
 */
void bus_queue_add(BusQueue *self, const BusEvent *ev);

/**
 * @brief Remove and return the oldest event from the queue.
 *
 * @param self The queue to pop from.
 * @param out  Output buffer where the removed event is written.
 * @return     @p out on success, NULL if the queue was empty.
 */
BusEvent *bus_queue_pop(BusQueue *self, BusEvent *out);

/**
 * @brief Return a pointer to the oldest event without removing it.
 *
 * Useful when the caller needs to inspect pointer fields inside the event
 * before deciding whether to consume it.
 *
 * @param self The queue to peek into.
 * @return     Pointer to the front event, or NULL if the queue is empty.
 */
BusEvent *bus_queue_peek_ref(BusQueue *self);

/**
 * @brief Copy the oldest event without removing it.
 *
 * @param self The queue to peek into.
 * @param out  Output buffer where the front event is copied.
 * @return     true if an event was copied, false if the queue was empty.
 */
bool bus_queue_peek(BusQueue *self, BusEvent *out);

/* =========================================================================
 * Bus reset callback
 * ========================================================================= */

/**
 * @brief Callback invoked by the bus after a system reset completes.
 *
 * The host firmware registers this callback to perform any platform-specific
 * work after the emulator resets (e.g. clearing the framebuffer or reloading
 * the BIOS ROM image).
 *
 * @param bus Pointer to the Bus that just reset.
 */
typedef void (*BusResetCallback)(Bus *bus);

/* =========================================================================
 * Bus struct
 * ========================================================================= */

/**
 * @brief The central system bus – owns and connects every emulator component.
 *
 * There is exactly one Bus instance per running NES system.  It is the only
 * struct that the host needs to allocate; every other component is embedded
 * inside it by value.  On the RP2350 this struct lives in SRAM.
 *
 * The bus serves two roles at once:
 *   1. **Communication hub** – routes CPU reads and writes to the correct
 *      component, triggers catch-up synchronisation, and handles OAM DMA.
 *   2. **Lifecycle manager** – initialises, resets, and destroys the
 *      components it owns.
 */
typedef struct Bus {
    /* -----------------------------------------------------------------
     * Emulated subsystems (owned by value)
     * ----------------------------------------------------------------- */

    Cpu    cpu;     /**< The 6502-derived Ricoh 2A03 CPU. */
    Ppu    ppu;     /**< The Picture Processing Unit (graphics). */
    Disk   disk;    /**< The loaded cartridge image and its active mapper. */
    Joypad joypad1; /**< Controller port 1 (player 1). */
    Joypad joypad2; /**< Controller port 2 (player 2). */
    Apu    apu;     /**< The Audio Processing Unit (sound). */
    Fwx    fwx;     /**< FirmWare eXchange – the communication bridge between
                      *   the emulated program (BIOS) and the host firmware.
                      *   Its registers are mapped at $4018-$401A in the CPU
                      *   address space (section 6.7 of the paper). */

    /* -----------------------------------------------------------------
     * Host-side resources
     * ----------------------------------------------------------------- */

    Fs *fs; /**< Filesystem handle used to open ROM files from the SD card.
              *   Not owned by the Bus; must remain valid for the full lifetime
              *   of the Bus instance. */

    /* -----------------------------------------------------------------
     * Internal RAM
     *
     * The NES has 2 KiB of internal RAM at $0000-$07FF.  The full range
     * $0000-$1FFF mirrors this block three times, which is handled in
     * bus_read() / bus_write() by masking the address with 0x07FF.
     * ----------------------------------------------------------------- */

    uint8_t memory[0x800]; /**< 2 KiB of internal CPU RAM ($0000-$07FF). */

    /* -----------------------------------------------------------------
     * State flags
     * ----------------------------------------------------------------- */

    bool disk_connected; /**< True when a cartridge image is loaded and the
                           *   mapper is active.  False on startup and after
                           *   bus_deinit() unloads the disk. */

    /* -----------------------------------------------------------------
     * Event queue
     * ----------------------------------------------------------------- */

    BusQueue ev_queue; /**< Pending bus events (ROM loads, resets, FWX errors).
                         *   Posted during emulation, drained between frames
                         *   by bus_update() at a safe synchronisation point. */

    /* -----------------------------------------------------------------
     * Host callbacks and context
     * ----------------------------------------------------------------- */

    void            *user_data;      /**< Arbitrary pointer forwarded to host callbacks.
                                       *   Not read or written by the bus itself. */
    BusResetCallback reset_cb;       /**< Optional callback fired after a system reset.
                                       *   Registered by the host firmware; may be NULL. */
    bool             reset_requested;/**< Set to true by the hardware reset-button ISR.
                                       *   The main firmware loop checks and clears this
                                       *   flag before each frame, then posts a
                                       *   BUS_EVENT_RESET to the queue. */
} Bus;

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief Initialise the Bus and all components it owns.
 *
 * Allocates no heap memory; all components are embedded in @p self by value.
 * After this call the system is in a cold-boot state with no cartridge loaded.
 * Call bus_load_disk() or bus_load_disk_mem() to load a ROM, then bus_reset()
 * to begin execution.
 *
 * @param self  The Bus struct to initialise.
 * @param fs    Filesystem handle for ROM loading; must outlive the Bus.
 * @return      true on success, false if any component failed to initialise.
 */
bool bus_init(Bus *self, Fs *fs);

/**
 * @brief Tear down the Bus and release any resources it owns.
 *
 * Unloads the cartridge (freeing CHR/PRG RAM if allocated) and resets all
 * component state.  After this call @p self must not be used until bus_init()
 * is called again.
 *
 * @param self The Bus to clean up.
 */
void bus_deinit(Bus *self);

/**
 * @brief Reset the emulated system (equivalent to pressing the Reset button).
 *
 * Reloads the reset vector from $FFFC/$FFFD into the CPU program counter,
 * clears PPU and APU state, and re-initialises FWX.  The cartridge ROM and
 * battery-backed PRG RAM are not cleared, matching real NES hardware.
 * Fires @ref Bus::reset_cb if one is registered.
 *
 * @param self The Bus to reset.
 */
void bus_reset(Bus *self);

/**
 * @brief Process all pending bus events queued since the last call.
 *
 * Should be called once per frame, before running the next frame of emulation,
 * so that deferred operations (ROM loads, resets, FWX error codes) are applied
 * at a safe point where the emulator state is fully consistent.
 *
 * @param self The Bus whose event queue should be drained.
 * @return     true if emulation should continue, false on a fatal error
 *             (e.g. the requested ROM file could not be opened).
 */
bool bus_update(Bus *self);

/**
 * @brief Post an event to the bus event queue.
 *
 * The event is copied into the queue.  On the RP2350 this function may be
 * called from an interrupt handler, but interrupts must be disabled around the
 * call to prevent races with bus_update() on the main loop (see the reset
 * button handler in the firmware, listing 13 of the paper).
 *
 * @param self The Bus that owns the queue.
 * @param ev   The event to enqueue (copied by value).
 */
void bus_add_event(Bus *self, const BusEvent *ev);

/**
 * @brief Write one byte to the CPU address space.
 *
 * Implements the full NES CPU memory map.  The top 4 bits of the address select
 * the destination via a fast switch/jump-table rather than a chain of if-else
 * comparisons:
 *   - $0000-$1FFF → internal RAM (with $0800-$1FFF mirroring $0000-$07FF)
 *   - $2000-$3FFF → PPU registers (PPU catches up first via ppu_run_until())
 *   - $4000-$401F → APU, I/O, OAM DMA ($4014), and FWX registers
 *   - $4020-$FFFF → cartridge space (forwarded to the mapper write hook)
 *
 * @param self    The Bus.
 * @param address 16-bit CPU address to write to.
 * @param data    The byte to write.
 */
void bus_write(Bus *self, uint16_t address, uint8_t data);

/**
 * @brief Read one byte from the CPU address space.
 *
 * Follows the same routing logic as bus_write().  Reads from PPU and FWX
 * registers have side-effects (e.g. reading $2002 clears the VBlank flag)
 * that are handled inside the respective component's read function.
 *
 * @param self    The Bus.
 * @param address 16-bit CPU address to read from.
 * @return        The byte at that address.
 */
uint8_t bus_read(Bus *self, uint16_t address);

/**
 * @brief Load a ROM file from the SD card and attach it as the active cartridge.
 *
 * Opens the .nes file at @p path using the Bus's filesystem handle, parses the
 * iNES header, initialises the correct mapper, and sets @ref Bus::disk_connected
 * to true.  Any previously loaded cartridge is unloaded first.
 *
 * Call bus_reset() after a successful load to start the game from the
 * beginning (load the reset vector and run from $FFFC/$FFFD).
 *
 * @param self The Bus.
 * @param path Null-terminated path to the .nes file on the SD card filesystem.
 * @return     true on success, false if the file could not be opened or parsed.
 */
bool bus_load_disk(Bus *self, const char *path);

/**
 * @brief Load a ROM from a memory buffer and attach it as the active cartridge.
 *
 * Useful for loading the BIOS image, which is embedded directly in the firmware
 * binary and does not need to be read from the SD card.
 *
 * @param self The Bus.
 * @param data Pointer to the raw ROM bytes (iNES format).
 * @param size Length of the ROM data in bytes.
 * @return     true on success, false if the data could not be parsed.
 */
bool bus_load_disk_mem(Bus *self, const uint8_t *data, unsigned size);

/**
 * @brief Signal the end of a PPU scanline to all relevant components.
 *
 * Called by the PPU at the end of each rendered scanline.  Gives the active
 * mapper a chance to update its IRQ scanline counter – this is the mechanism
 * MMC3 uses to generate mid-frame interrupts for split-screen scrolling effects
 * and status bars (section 6.3.1 of the paper).
 *
 * Internally forwards the call to the mapper's @ref MapperHsyncFn hook stored
 * in @ref Disk::mapper_hsync.
 *
 * @param self The Bus.
 */
void bus_hsync(Bus *self);

#ifdef __cplusplus
}
#endif