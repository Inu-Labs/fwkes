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

#include <fwkes/apu.h>

#include <fwkes/bus.h>
#include <fwkes/cpu.h>
#include <fwkes/util.h>

#include <string.h>

#define FRAME_CNT_FREQ 240
#define CYCLES_PER_QUARTER_FRAME (CPU_FREQ / FRAME_CNT_FREQ)
/* We'll use 16.16 fixed-point arithmetic for sample accumulation. */
#define FIXED_CYCLES_PER_SAMPLE ((CPU_FREQ_U << 16) / SAMPLE_RATE)

static const uint8_t g_len_lut[32] = {10,  254, 20, 2,  40, 4,  80, 6,
                                      160, 8,   60, 10, 14, 12, 26, 14,
                                      12,  16,  24, 18, 48, 20, 96, 22,
                                      192, 24,  72, 26, 16, 28, 32, 30};
static const uint8_t g_duty_lut[4][8] = {
    {0, 1, 0, 0, 0, 0, 0, 0},
    {0, 1, 1, 0, 0, 0, 0, 0},
    {0, 1, 1, 1, 1, 0, 0, 0},
    {1, 0, 0, 1, 1, 1, 1, 1}
};
static const uint16_t g_noise_periods[16] = {4,   8,    16,   32,  64,  96,
                                             128, 160,  202,  254, 380, 508,
                                             762, 1016, 2034, 4068};
static const uint16_t g_dmc_periods[16] = {428, 380, 340, 320, 286, 254,
                                           226, 214, 190, 160, 142, 128,
                                           106, 84,  72,  54};

static ApuSample g_pulse_lut[31];
static ApuSample g_tnd_lut[203];

void apu_init(Apu *self, Bus *bus) {
    memset(self, 0, sizeof(Apu));

    self->bus = bus;
    self->noise.shift_reg = 1;
    self->sample_step = (uint32_t) ((CPU_FREQ_U << 16) / SAMPLE_RATE);
    self->noise.timer.period = 4;
    self->noise.timer.value = 4;
    self->dmc.buf_empty = true;
    self->dmc.bits_rem = 8;
    self->dmc.silence = true;
    self->dmc.timer.value = 0;

    /* https://www.nesdev.org/wiki/APU_Mixer#Lookup_Table */

    for (int i = 0; i < 31; ++i) {
        g_pulse_lut[i] = (ApuSample) ((95.52 / (8128.0 / (double) i + 100.0)) *
                                      APU_SAMPLE_MAX_VALUE);
    }

    for (int i = 0; i < 203; ++i) {
        g_tnd_lut[i] = (ApuSample) ((163.67 / (24329.0 / (double) i + 100)) *
                                    APU_SAMPLE_MAX_VALUE);
    }
}

void apu_reset(Apu *self) {
    ApuSampleCallback tmp_smp_cb = self->sample_cb;
    void *tmp_user_data = self->user_data;
    Bus *tmp_bus = self->bus;

    memset(self, 0, sizeof(*self));
    self->dmc.buf_empty = true;
    self->dmc.bits_rem = 8;
    self->dmc.silence = true;
    self->dmc.timer.value = 0;
    self->bus = tmp_bus;
    self->user_data = tmp_user_data;
    self->sample_cb = tmp_smp_cb;
    self->noise.shift_reg = 1;
    self->sample_step = (uint32_t) ((CPU_FREQ_U << 16) / SAMPLE_RATE);
    self->noise.timer.period = 4;
    self->noise.timer.value = 4;
}

FORCE_INLINE void tick_envelope(ApuEnvelope *env, bool loop) {
    if (env->start) {
        env->start = false;
        env->decay_counter = 15;
        env->divider.value = env->volume;
    } else {
        if (env->divider.value == 0) {
            env->divider.value = env->volume;

            if (env->decay_counter > 0) {
                env->decay_counter--;
            } else if (loop) {
                env->decay_counter = 15;
            }
        } else {
            env->divider.value--;
        }
    }
    env->output = env->const_volume ? env->volume : env->decay_counter;
}

FORCE_INLINE void tick_sweep(PulseChannel *p) {
    ApuSweep *s = &p->sweep;

    uint16_t curr = p->timer.period;
    bool mute = (curr < 8) || (curr > 0x7FF);

    if (s->divider.value == 0 && s->on && s->shift > 0 && !mute) {
        int delta = curr >> s->shift;
        if (s->negate) {
            curr -= delta;
            if (s->negate)
                curr -= 1;
        } else {
            curr += delta;
        }

        if (curr > 0x7FF)
            mute = true;
        else if (curr >= 8) {
            p->timer.period = curr;
        }
    }

    if (s->divider.value == 0 || s->reload) {
        s->divider.value = s->period;
        s->reload = false;
    } else {
        s->divider.value--;
    }

    s->mute = mute;
}

FORCE_INLINE void tick_linear_counter(TriangleChannel *tri) {
    if (tri->linear.reload) {
        tri->linear.value = tri->linear.reload_val;
    } else if (tri->linear.value > 0) {
        tri->linear.value--;
    }

    if (!tri->linear.control) {
        tri->linear.reload = false;
    }
}

FORCE_INLINE void tick_length(ApuLengthCounter *lc) {
    if (lc->on && lc->value > 0) {
        lc->value--;
    }
}

static void apu_clock_frame(Apu *self) {
    ++self->frame.step;

    if (self->frame.step > (self->frame.mode_5_step ? 5 : 4)) {
        self->frame.step = 1;
    }

    bool quarter = false;
    bool half = false;
    int s = self->frame.step;

    if (self->frame.mode_5_step) {
        if (s == 1 || s == 2 || s == 3 || s == 5) {
            quarter = true;
        }

        if (s == 2 || s == 5) {
            half = true;
        }
    } else {
        quarter = true;

        if (s == 2 || s == 4) {
            half = true;
        }
    }

    if (quarter) {
        tick_envelope(&self->p1.env, self->p1.len.on == 0);
        tick_envelope(&self->p2.env, self->p2.len.on == 0);
        tick_envelope(&self->noise.env, self->noise.len.on == 0);
        tick_linear_counter(&self->tri);
    }

    if (half) {
        tick_length(&self->p1.len);
        tick_length(&self->p2.len);
        tick_length(&self->tri.len);
        tick_length(&self->noise.len);
        tick_sweep(&self->p1);
        tick_sweep(&self->p2);
    }

    if (!self->frame.irq_inhibit && !self->frame.mode_5_step &&
        self->frame.step >= 4) {
        cpu_irq_pulldown(&self->bus->cpu, IRQ_FRAME_CNT, true);
    }
}

static inline void dmc_fetch(DmcChannel *dmc, Bus *bus) {
    if (dmc->buf_empty && dmc->bytes_rem > 0) {
        dmc->buf = bus_read(bus, dmc->curr_addr);

        dmc->curr_addr++;
        if (dmc->curr_addr == 0)
            dmc->curr_addr = 0x8000;

        dmc->bytes_rem--;
        dmc->buf_empty = false;

        if (dmc->bytes_rem == 0) {
            if (dmc->loop) {
                dmc->curr_addr = dmc->sample_addr;
                dmc->bytes_rem = dmc->sample_len;
            } else if (dmc->irq_on) {
            }
        }
    }
}

void NOTFLASH_FN(apu_run_until)(Apu *self, CycleCounter target_cycles) {
    CycleDiff cycles_to_run = (CycleDiff) (target_cycles - self->cycles);
    if (cycles_to_run <= 0)
        return;

    /* The reason this code looks so messy and ugly is because we unrolled
     * loops, and in general we try to keep everything very linear and
     * lightweight. */
    while (cycles_to_run >= 2) {
        if (self->tri.timer.value == 0) {
            self->tri.timer.value = self->tri.timer.period;
            if (self->tri.len.value > 0 && self->tri.linear.value > 0) {
                self->tri.seq_index = (self->tri.seq_index + 1) & 31;
            }
        } else
            self->tri.timer.value--;

        if (self->tri.timer.value == 0) {
            self->tri.timer.value = self->tri.timer.period;
            if (self->tri.len.value > 0 && self->tri.linear.value > 0) {
                self->tri.seq_index = (self->tri.seq_index + 1) & 31;
            }
        } else
            self->tri.timer.value--;

        if (self->p1.timer.value == 0) {
            self->p1.timer.value = self->p1.timer.period;
            self->p1.duty_pos = (self->p1.duty_pos + 1) & 7;
        } else
            self->p1.timer.value--;

        if (self->p2.timer.value == 0) {
            self->p2.timer.value = self->p2.timer.period;
            self->p2.duty_pos = (self->p2.duty_pos + 1) & 7;
        } else
            self->p2.timer.value--;

        if (self->noise.timer.value <= 1) { /* Underflow check for 2 cycles */
            while (self->noise.timer.value <= 1) {
                uint16_t feedback =
                    (self->noise.shift_reg & 1) ^
                    ((self->noise.shift_reg >> (self->noise.mode ? 6 : 1)) & 1);
                self->noise.shift_reg >>= 1;
                self->noise.shift_reg |= (feedback << 14);

                self->noise.timer.value += self->noise.timer.period;
            }
            self->noise.timer.value -= 2;
        } else {
            self->noise.timer.value -= 2;
        }
        if (self->dmc.on) {
            dmc_fetch(&self->dmc, self->bus);

            if (self->dmc.timer.period != 0) {
                if (self->dmc.timer.value <= 2) {
                    while (self->dmc.timer.value <= 2) {
                        self->dmc.timer.value += self->dmc.timer.period;

                        if (!self->dmc.silence) {
                            if (self->dmc.shift_reg & 1) {
                                if (self->dmc.out_level <= 125)
                                    self->dmc.out_level += 2;
                            } else {
                                if (self->dmc.out_level >= 2)
                                    self->dmc.out_level -= 2;
                            }
                        }

                        self->dmc.shift_reg >>= 1;
                        self->dmc.bits_rem--;

                        if (self->dmc.bits_rem == 0) {
                            self->dmc.bits_rem = 8;

                            if (self->dmc.buf_empty) {
                                self->dmc.silence = true;
                            } else {
                                self->dmc.silence = false;
                                self->dmc.shift_reg = self->dmc.buf;
                                self->dmc.buf_empty = true;
                            }
                        }
                    }

                    self->dmc.timer.value -= 2;
                } else {
                    self->dmc.timer.value -= 2;
                }
            }
        }

        self->frame.cycle_countdown -= 2;
        if (self->frame.cycle_countdown <= 0) {
            self->frame.cycle_countdown += 7457;
            apu_clock_frame(self);
        }

        self->sample_acc += (2 << 16); /* +2.0 CPU cycles */
        if (self->sample_acc >= self->sample_step) {
            self->sample_acc -= self->sample_step;

            if (self->sample_cb) {
                uint8_t p1_out =
                    (self->p1.len.value > 0 && !self->p1.sweep.mute &&
                     g_duty_lut[self->p1.duty][self->p1.duty_pos])
                        ? self->p1.env.output
                        : 0;
                uint8_t p2_out =
                    (self->p2.len.value > 0 && !self->p2.sweep.mute &&
                     g_duty_lut[self->p2.duty][self->p2.duty_pos])
                        ? self->p2.env.output
                        : 0;

                static const uint8_t tri_seq[32] = {
                    15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5,  4,  3,  2,  1,  0,
                    0,  1,  2,  3,  4,  5,  6, 7, 8, 9, 10, 11, 12, 13, 14, 15
                };

                uint8_t n_out =
                    (self->noise.len.value > 0 && !(self->noise.shift_reg & 1))
                        ? self->noise.env.output
                        : 0;

                uint8_t tri_out = (self->tri.timer.period >= 2)
                                      ? tri_seq[self->tri.seq_index]
                                      : 0;
                uint8_t d_out = self->dmc.out_level;

                ApuSample pulse_smp = g_pulse_lut[p1_out + p2_out];
                ApuSample tnd_smp = g_tnd_lut[3 * tri_out + 2 * n_out + d_out];

                self->sample_cb(self, pulse_smp + tnd_smp);
            }
        }

        cycles_to_run -= 2;
        self->cycles += 2;
    }
}

void NOTFLASH_FN(apu_write)(Apu *self, uint16_t addr, uint8_t data) {
    switch (addr) {
    case 0x4000:
        self->p1.duty = (data >> 6) & 3;
        self->p1.len.on = !(data & 0x20);
        self->p1.env.loop = (data & 0x20);
        self->p1.env.const_volume = (data & 0x10);
        self->p1.env.volume = data & 0x0F;

        break;
    case 0x4001:
        self->p1.sweep.on = data & 0x80;
        self->p1.sweep.period = ((data >> 4) & 7) + 1;
        self->p1.sweep.negate = data & 0x08;
        self->p1.sweep.shift = data & 0x07;
        self->p1.sweep.reload = true;

        break;
    case 0x4002:
        self->p1.timer.period = (self->p1.timer.period & 0x700) | data;

        break;
    case 0x4003:
        self->p1.timer.period = (uint16_t) (self->p1.timer.period & 0xFF) |
                                (uint16_t) ((data & 7) << 8);

        if (self->p1.on) {
            self->p1.len.value = g_len_lut[data >> 3];
        }

        self->p1.duty_pos = 0;
        self->p1.env.start = true;

        break;
    case 0x4004:
        self->p2.duty = (data >> 6) & 3;
        self->p2.len.on = !(data & 0x20);
        self->p2.env.loop = (data & 0x20);
        self->p2.env.const_volume = (data & 0x10);
        self->p2.env.volume = data & 0x0F;

        break;
    case 0x4005:
        self->p2.sweep.on = data & 0x80;
        self->p2.sweep.period = ((data >> 4) & 7) + 1;
        self->p2.sweep.negate = data & 0x08;
        self->p2.sweep.shift = data & 0x07;
        self->p2.sweep.reload = true;

        break;
    case 0x4006:
        self->p2.timer.period = (self->p2.timer.period & 0x700) | data;

        break;
    case 0x4007:
        self->p2.timer.period = (uint16_t) (self->p2.timer.period & 0xFF) |
                                (uint16_t) ((data & 7) << 8);
        if (self->p2.on) {
            self->p2.len.value = g_len_lut[data >> 3];
        }

        self->p2.duty_pos = 0;
        self->p2.env.start = true;

        break;
    case 0x4008:
        self->tri.linear.control = data & 0x80;
        self->tri.linear.reload_val = data & 0x7F;

        break;
    case 0x400A:
        self->tri.timer.period = (self->tri.timer.period & 0x700) | data;

        break;
    case 0x400B:
        self->tri.timer.period = (uint16_t) (self->tri.timer.period & 0xFF) |
                                 (uint16_t) ((data & 7) << 8);
        if (self->tri.on) {
            self->tri.len.value = g_len_lut[data >> 3];
        }

        self->tri.linear.reload = true;

        break;
    case 0x400C:
        self->noise.len.on = !(data & 0x20);
        self->noise.env.loop = (data & 0x20);
        self->noise.env.const_volume = (data & 0x10);
        self->noise.env.volume = data & 0x0F;

        break;
    case 0x400E:
        self->noise.mode = data & 0x80;
        self->noise.timer.period = g_noise_periods[data & 0x0F];

        break;
    case 0x400F:
        if (self->noise.on) {
            self->noise.len.value = g_len_lut[data >> 3];
        }

        self->noise.env.start = true;

        break;
    case 0x4010:
        self->dmc.irq_on = data & 0x80;
        self->dmc.loop = data & 0x40;
        self->dmc.rate = g_dmc_periods[data & 0xF];
        self->dmc.timer.period = self->dmc.rate;

        break;
    case 0x4011:
        self->dmc.out_level = data & 0x7F;

        break;
    case 0x4012:
        self->dmc.sample_addr = 0xC000 + (data * 64);

        break;
    case 0x4013:
        self->dmc.sample_len = (data * 16) + 1;

        break;
    case 0x4015:
        self->p1.on = data & 1;
        self->p2.on = data & 2;
        self->tri.on = data & 4;
        self->noise.on = data & 8;

        bool dmc_enable = data & 16;

        if (dmc_enable) {
            if (self->dmc.bytes_rem == 0) {
                self->dmc.curr_addr = self->dmc.sample_addr;
                self->dmc.bytes_rem = self->dmc.sample_len;

                self->dmc.bits_rem = 8;
                self->dmc.buf_empty = true;
                self->dmc.timer.value = self->dmc.timer.period;
            }
        } else {
            self->dmc.bytes_rem = 0;
        }

        self->dmc.on = dmc_enable;

        /* clang-format off */
        if (!self->p1.on) self->p1.len.value = 0;
        if (!self->p2.on) self->p2.len.value = 0;
        if (!self->tri.on) self->tri.len.value = 0;
        if (!self->noise.on) self->noise.len.value = 0;
        /* clang-format on */

        break;
    case 0x4017:
        self->frame.mode_5_step = data & 0x80;
        self->frame.irq_inhibit = data & 0x40;

        if (self->frame.irq_inhibit) {
            cpu_irq_pulldown(&self->bus->cpu, IRQ_FRAME_CNT, false);
        }

        self->frame.step = 0;
        self->frame.cycle_countdown = 7457;

        if (self->frame.mode_5_step) {
            apu_clock_frame(self);
            tick_envelope(&self->p1.env, false);
            tick_envelope(&self->p2.env, false);
            tick_envelope(&self->noise.env, false);
            tick_linear_counter(&self->tri); /* Immediate tick */
        }

        break;
    }
}

uint8_t apu_read(Apu *apu, uint16_t addr) { return apu_peek(apu, addr); }

uint8_t apu_peek(const Apu *self, uint16_t addr) {
    if (addr == 0x4015) {
        bool pulse_1 = self->p1.len.value > 0;
        bool pulse_2 = self->p2.len.value > 0;
        bool triangle = self->tri.len.value > 0;
        bool noise = self->noise.len.value > 0;
        uint8_t channels = pulse_1 | (uint8_t) (pulse_2 << 1) |
                           (uint8_t) (triangle << 2) | (uint8_t) (noise << 3);
        bool dmc = self->dmc.on;
        bool frame_irq = self->frame.irq_active;
        bool dmc_irq = self->dmc.irq_on;

        cpu_irq_pulldown(&self->bus->cpu, IRQ_FRAME_CNT, false);

        return channels | (uint8_t) (dmc << 4) | (uint8_t) (frame_irq << 6) |
               (uint8_t) (dmc_irq << 7);
    }

    return 0x00;
}
