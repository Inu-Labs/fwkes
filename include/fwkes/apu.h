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
 * @file apu.h
 * @brief NES Audio Processing Unit (APU) – five-channel sound synthesis.
 *
 * The APU is integrated into the 2A03 CPU die and provides two pulse channels,
 * a triangle channel, a noise channel, and a DPCM sample channel (DMC).
 * Output samples are delivered to the host via ApuSampleCallback at SAMPLE_RATE.
 * Timing is driven by the catch-up mechanism: apu_run_until() fast-forwards
 * the APU to match the current CPU cycle count.
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "util.h"

/** Output sample rate in Hz. 31 250 on RP2350 (integer divisor of the 250 kHz
 *  PWM carrier), 44 100 on the desktop SDL3 backend. */
#ifdef BUILD_RP2350
#    define SAMPLE_RATE 31250
#else
#    define SAMPLE_RATE 44100
#endif

typedef struct Bus Bus;

/** Countdown timer used throughout the APU.
 *  Fires and reloads from @ref period every (period + 1) clock ticks. */
typedef struct {
    uint16_t period; /**< Reload value (also called reload_value in some docs). */
    uint16_t value;  /**< Current countdown; decremented each tick. */
} ApuDivider;

/** Automatic note-length counter (Pulse, Triangle, Noise).
 *  Decrements once per frame-counter tick; silences the channel when it hits 0. */
typedef struct {
    bool    on;           /**< false = counter halted (channel not silenced by it). */
    uint8_t value;        /**< Current countdown; 0 = channel silenced. */
    uint8_t reload_value; /**< Value to load on next reload. */
} ApuLengthCounter;

/** Volume envelope generator (Pulse, Noise).
 *  Produces a decaying sawtooth volume curve; can loop or output a constant level. */
typedef struct {
    bool    start;         /**< Restart the envelope from 15 on next frame-counter tick. */
    bool    loop;          /**< Wrap back to 15 instead of silencing when decay reaches 0. */
    bool    const_volume;  /**< Use @ref volume as a fixed level instead of the decayed value. */
    uint8_t volume;        /**< Constant volume level (used when const_volume is true). */
    uint8_t decay_counter; /**< Internal 4-bit decay counter (0-15). */
    uint8_t output;        /**< Current envelope output fed to the channel. */
    ApuDivider divider;    /**< Clocks the decay step; period written by CPU. */
} ApuEnvelope;

/** Automatic pitch-sweep unit (Pulse channels only).
 *  Periodically shifts the channel timer period up or down, muting if out of range. */
typedef struct {
    bool    on;      /**< Sweep enabled. */
    bool    negate;  /**< true = subtract shift (pitch falls); false = add (rises). */
    bool    reload;  /**< Reload the divider on the next frame-counter tick. */
    uint8_t shift;   /**< Bit-shift amount applied to the current period each step. */
    uint8_t period;  /**< Divider reload value; controls sweep speed. */
    ApuDivider divider;
    bool    mute;    /**< true when the resulting period is out of the valid range. */
} ApuSweep;

/** State of one Pulse (square-wave) channel.
 *  The NES has two: Pulse 1 ($4000-$4003) and Pulse 2 ($4004-$4007). */
typedef struct {
    bool    on;       /**< Channel enabled ($4015). */
    uint8_t duty;     /**< Duty cycle index 0-3: 12.5%, 25%, 50%, 25% negated. */
    uint8_t duty_pos; /**< Current position in the 8-step duty sequence. */
    ApuDivider      timer;
    ApuLengthCounter len;
    ApuEnvelope     env;
    ApuSweep        sweep;
    uint8_t output;
} PulseChannel;

/** Linear counter specific to the Triangle channel.
 *  Additional fine-grained length control; clocked twice as fast as ApuLengthCounter. */
typedef struct ApuLinearCounter {
    bool    control;    /**< When true the counter is halted (does not decrement). */
    bool    reload;     /**< Reload from reload_val on the next tick. */
    uint8_t reload_val; /**< 7-bit reload value written via $4008. */
    uint8_t value;      /**< Current countdown; 0 silences the channel. */
} ApuLinearCounter;

/** State of the Triangle wave channel ($4008, $400A-$400B).
 *  Produces a fixed-volume 32-step triangle waveform at the full CPU clock rate. */
typedef struct {
    bool    on;
    ApuDivider       timer;
    ApuLengthCounter len;
    ApuLinearCounter linear;
    uint8_t seq_index; /**< Current position in the 32-step triangle sequence. */
    uint8_t output;
} TriangleChannel;

/** State of the Noise channel ($400C, $400E-$400F).
 *  Pseudo-random noise generated by a 15-bit LFSR. */
typedef struct {
    bool     on;
    bool     mode;      /**< false = 15-bit random; true = periodic (bit-6 feedback tap). */
    uint16_t shift_reg; /**< 15-bit LFSR; initialised to 1 on reset. */
    ApuDivider       timer;
    ApuLengthCounter len;
    ApuEnvelope      env;
    uint8_t output;
} NoiseChannel;

/** State of the DMC (Delta Modulation Channel, $4010-$4013).
 *  Plays back 1-bit DPCM samples from PRG ROM; each bit steps the output level up or down. */
typedef struct {
    bool     on;
    bool     irq_on;      /**< Generate an IRQ when the sample buffer runs out. */
    bool     loop;        /**< Restart playback automatically at the end of the sample. */
    uint16_t rate;        /**< Timer period (from lookup table, written via $4010 bits 0-3). */
    uint16_t sample_addr; /**< Start address of the sample in CPU space. */
    uint16_t sample_len;  /**< Length of the sample in bytes. */
    uint16_t curr_addr;   /**< Address of the next byte to fetch via DMA. */
    uint16_t bytes_rem;   /**< Bytes remaining before playback ends. */
    uint8_t  buf;         /**< Current sample byte being shifted out. */
    bool     buf_empty;   /**< True when buf is exhausted and a new DMA fetch is needed. */
    uint8_t  shift_reg;   /**< 8-bit shift register; one bit consumed per timer tick. */
    uint8_t  bits_rem;    /**< Bits remaining in shift_reg (0-8). */
    uint8_t  out_level;   /**< 7-bit output level (0-127) driven up/down by DPCM bits. */
    bool     silence;     /**< No sample data available; output level held still. */
    ApuDivider timer;
} DmcChannel;

/** Maximum absolute value of a mixed output sample (INT16_MAX). */
#ifdef BUILD_RP2350
#    define APU_SAMPLE_MAX_VALUE INT16_MAX
#else
#    define APU_SAMPLE_MAX_VALUE INT16_MAX
#endif

/** Type of one mixed output audio sample (signed 16-bit on both targets). */
#ifdef BUILD_RP2350
typedef int16_t ApuSample;
#else
typedef int16_t ApuSample;
#endif

typedef struct Apu Apu;

/** Called by the APU each time a new output sample is ready.
 *  The host backend writes it to the DMA audio buffer (RP2350) or SDL stream (desktop). */
typedef void (*ApuSampleCallback)(Apu *self, ApuSample s);

/** State of the APU frame counter (sequencer).
 *  Clocks envelopes, sweeps, and length counters at ~240 Hz (4-step) or ~192 Hz (5-step). */
typedef struct ApuFrame {
    bool mode_5_step;     /**< false = 4-step mode (frame IRQ possible); true = 5-step. */
    bool irq_inhibit;     /**< Suppress frame IRQ even in 4-step mode. */
    bool irq_active;      /**< Currently asserting frame IRQ on the CPU IRQ line. */
    int  step;            /**< Current sequencer step (0-3 or 0-4). */
    int  cycle_countdown; /**< CPU cycles until the next sequencer step fires. */
} ApuFrame;

/** Complete state of the emulated NES APU.
 *  One instance lives inside the Bus struct. */
typedef struct Apu {
    PulseChannel    p1;    /**< Pulse channel 1 ($4000-$4003). */
    PulseChannel    p2;    /**< Pulse channel 2 ($4004-$4007). */
    TriangleChannel tri;   /**< Triangle channel ($4008, $400A-$400B). */
    NoiseChannel    noise; /**< Noise channel ($400C, $400E-$400F). */
    DmcChannel      dmc;   /**< DMC channel ($4010-$4013). */
    ApuFrame        frame; /**< Frame sequencer. */

    CycleCounter cycles;          /**< Total APU cycles elapsed since reset. */
    CycleCounter last_sync_cycle; /**< Cycle at which the APU was last synchronised. */
    uint32_t     sample_acc;      /**< 16.16 fixed-point cycle accumulator for downsampling. */
    uint32_t     sample_step;     /**< 16.16 fixed-point cycles-per-sample (CPU_FREQ / SAMPLE_RATE). */
    ApuSampleCallback sample_cb;  /**< Host callback invoked once per output sample. */
    void        *user_data;       /**< Forwarded to sample_cb; not used by the APU itself. */
    Bus         *bus;             /**< Back-pointer to the bus for DMC DMA reads. */
} Apu;

/** Initialise the APU and pre-compute the mixer LUT tables.
 *  Must be called once before any other apu_* function. */
void apu_init(Apu *apu, Bus *bus);

/** Reset APU to power-on state (silences all channels, resets frame counter). */
void apu_reset(Apu *apu);

/** Write a byte to an APU register ($4000-$4013, $4015, $4017). */
void apu_write(Apu *apu, uint16_t addr, uint8_t data);

/** Read an APU register with side-effects (only $4015 returns meaningful data). */
uint8_t apu_read(Apu *apu, uint16_t addr);

/** Read an APU register without side-effects (for debuggers / savestates). */
uint8_t apu_peek(const Apu *apu, uint16_t addr);

/** Catch-up: advance APU state until @p target_cycles, emitting samples via sample_cb. */
void apu_run_until(Apu *apu, CycleCounter target_cycles);
