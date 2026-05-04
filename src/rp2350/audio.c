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

#include <fwkes/rp2350/audio.h>

#include <hardware/clocks.h>
#include <hardware/dma.h>
#include <hardware/irq.h>
#include <hardware/pwm.h>

#define RING_SIZE 2
#define PWM_VALUE(smp) (uint16_t) ((((uint32_t) (smp) * g_pwm_wrap) / 65535))

static unsigned g_audio_chan;
static unsigned g_ctrl_chan;
static unsigned g_dma_timer;
static unsigned g_pwm_slice;
static uint16_t g_pwm_wrap;
static volatile uint16_t g_bufs[RING_SIZE][AUDIO_BUFFER_SIZE];
static volatile unsigned g_curr_buf_idx = 0;
static volatile unsigned g_buf_pos;
static volatile uint16_t *g_reload_addrs[RING_SIZE]
    __attribute__((aligned(RING_SIZE * sizeof(uint16_t *))));

FORCE_INLINE void init_pwm(void) {
    gpio_set_function(AUDIO_MONO_PIN, GPIO_FUNC_PWM);

    g_pwm_slice = pwm_gpio_to_slice_num(AUDIO_MONO_PIN);

    unsigned sys_clk = clock_get_hz(clk_sys);
    unsigned pwm_freq = 250000;
    g_pwm_wrap = (sys_clk / pwm_freq - 1) & 0xffff;

    pwm_set_wrap(g_pwm_slice, g_pwm_wrap);
    pwm_set_clkdiv(g_pwm_slice, 1.f);
    pwm_set_enabled(g_pwm_slice, true);
}

void audio_init(void) {
    for (int i = 0; i < AUDIO_BUFFER_SIZE; ++i) {
        g_bufs[0][i] = PWM_VALUE(0);
        g_bufs[1][i] = PWM_VALUE(0);
    }

    for (int i = 0; i < RING_SIZE; ++i) {
        g_reload_addrs[i] = (uint16_t *) ((uintptr_t) g_bufs[i] | 0x20000000);
    }

    init_pwm();

    g_dma_timer = (unsigned) dma_claim_unused_timer(true);
    /* DMA timer use formula sys_clk * (numerator / denominator).
     * But we can't directly pass sys_clk, since it can't fit into uint16_t (300
     * MHz). The workaround is to simplify the fraction (31.25kHz/300MHz =
     * 1/9600). dma_timer_set_fraction(g_dma_timer, SAMPLE_RATE, sys_clk); */
    dma_timer_set_fraction(g_dma_timer, 1, 9600);

    g_audio_chan = (unsigned) dma_claim_unused_channel(true);
    g_ctrl_chan = (unsigned) dma_claim_unused_channel(true);

    dma_channel_config audio_cfg = dma_channel_get_default_config(g_audio_chan);
    dma_channel_config ctrl_cfg = dma_channel_get_default_config(g_ctrl_chan);

    channel_config_set_transfer_data_size(&audio_cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&audio_cfg, true);
    channel_config_set_write_increment(&audio_cfg, false);
    channel_config_set_dreq(&audio_cfg, dma_get_timer_dreq(g_dma_timer));
    channel_config_set_chain_to(&audio_cfg, g_ctrl_chan);
    channel_config_set_high_priority(&audio_cfg, true);

    dma_channel_configure(
        g_audio_chan, &audio_cfg, (void *) &pwm_hw->slice[g_pwm_slice].cc,
        g_bufs[0], AUDIO_BUFFER_SIZE, false
    );

    channel_config_set_transfer_data_size(&ctrl_cfg, DMA_SIZE_32);
    channel_config_set_read_increment(&ctrl_cfg, true);
    channel_config_set_write_increment(&ctrl_cfg, false);
    /* Address wrapping to 2 words (2^3 = 8 bytes). We have 2 pointers, both
     * are 4 bytes big. */
    channel_config_set_ring(&ctrl_cfg, false, 3);
    channel_config_set_chain_to(&ctrl_cfg, g_audio_chan);
    channel_config_set_high_priority(&ctrl_cfg, true);

    dma_channel_configure(
        g_ctrl_chan, &ctrl_cfg, &dma_hw->ch[g_audio_chan].al3_read_addr_trig,
        g_reload_addrs, 1, true
    );
}

void audio_queue(ApuSample smp) {
    g_bufs[g_curr_buf_idx][g_buf_pos++] = PWM_VALUE(smp);

    if (g_buf_pos >= AUDIO_BUFFER_SIZE) {
        unsigned next_buf_idx = (g_curr_buf_idx + 1) & (RING_SIZE - 1);

        for (;;) {
            uintptr_t dma_addr = dma_hw->ch[g_audio_chan].read_addr;
            uintptr_t next_buf_start =
                (uintptr_t) g_bufs[next_buf_idx] | 0x20000000;
            uintptr_t next_buf_end = next_buf_start + sizeof(g_bufs[0]);

            if (dma_addr < next_buf_start || dma_addr >= next_buf_end) {
                break;
            }

            tight_loop_contents();
        }

        g_curr_buf_idx = next_buf_idx;
        g_buf_pos = 0;
    }
}
