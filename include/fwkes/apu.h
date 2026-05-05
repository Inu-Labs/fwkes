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

#include <stdbool.h>
#include <stdint.h>

#include "util.h"

#ifdef BUILD_RP2350
#    define SAMPLE_RATE 31250
#else
#    define SAMPLE_RATE 44100
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Bus Bus;

typedef struct {
    uint16_t period; /* alternative name: reload_value */
    uint16_t value;
} ApuDivider;

typedef struct {
    bool on;
    uint8_t value;
    uint8_t reload_value;
} ApuLengthCounter;

typedef struct {
    bool start;
    bool loop;
    bool const_volume;
    uint8_t volume;
    uint8_t decay_counter;
    uint8_t output;
    ApuDivider divider;
} ApuEnvelope;

typedef struct {
    bool on;
    bool negate;
    bool reload;
    uint8_t shift;
    uint8_t period;
    ApuDivider divider;
    bool mute;
} ApuSweep;

typedef struct {
    bool on;
    uint8_t duty;
    uint8_t duty_pos;
    ApuDivider timer;
    ApuLengthCounter len;
    ApuEnvelope env;
    ApuSweep sweep;
    uint8_t output;
} PulseChannel;

typedef struct ApuLinearCounter {
    bool control;
    bool reload;
    uint8_t reload_val;
    uint8_t value;
} ApuLinearCounter;

typedef struct {
    bool on;
    ApuDivider timer;
    ApuLengthCounter len;
    ApuLinearCounter linear;
    uint8_t seq_index;
    uint8_t output;
} TriangleChannel;

typedef struct {
    bool on;
    bool mode;
    uint16_t shift_reg;
    ApuDivider timer;
    ApuLengthCounter len;
    ApuEnvelope env;
    uint8_t output;
} NoiseChannel;

typedef struct {
    bool on;
    bool irq_on;
    bool loop;
    uint16_t rate;
    uint16_t sample_addr;
    uint16_t sample_len;
    uint16_t curr_addr;
    uint16_t bytes_rem;
    uint8_t buf;
    bool buf_empty;
    uint8_t shift_reg;
    uint8_t bits_rem;
    uint8_t out_level;
    bool silence;
    ApuDivider timer;
} DmcChannel;

#ifdef BUILD_RP2350
#    define APU_SAMPLE_MAX_VALUE INT16_MAX
typedef int16_t ApuSample;
#else
#    define APU_SAMPLE_MAX_VALUE INT16_MAX
typedef int16_t ApuSample;
#endif

typedef struct Apu Apu;
typedef void (*ApuSampleCallback)(Apu *self, ApuSample s);

typedef struct ApuFrame {
    bool mode_5_step;
    bool irq_inhibit;
    bool irq_active;
    int step;
    int cycle_countdown;
} ApuFrame;

typedef struct Apu {
    PulseChannel p1;
    PulseChannel p2;
    TriangleChannel tri;
    NoiseChannel noise;
    DmcChannel dmc;
    ApuFrame frame;

    CycleCounter cycles;
    CycleCounter last_sync_cycle;
    uint32_t sample_acc; /* Fixed point 16.16 */
    uint32_t sample_step;
    ApuSampleCallback sample_cb;
    void *user_data;
    Bus *bus;
} Apu;

void apu_init(Apu *apu, Bus *bus);
void apu_reset(Apu *apu);
void apu_write(Apu *apu, uint16_t addr, uint8_t data);
uint8_t apu_read(Apu *apu, uint16_t addr);
uint8_t apu_peek(const Apu *apu, uint16_t addr);
void apu_run_until(Apu *apu, CycleCounter target_cycles);

#ifdef __cplusplus
}
#endif
