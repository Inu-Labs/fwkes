#ifndef FWX_PUBLIC_H
#define FWX_PUBLIC_H

#include "fwx_common.h"

#include <stdbool.h>
#include <stdint.h>

#define FWXSTAT *((unsigned char *) 0x4018)
#define FWXCTRL *((unsigned char *) 0x4019)
#define FWXDATA *((unsigned char *) 0x401a)

static void fwx_start(void) { FWXCTRL = (unsigned char) FWX_CTRL_SS; }

static void fwx_stop(void) { FWXCTRL = 0; }

static bool fwx_tx_done(void) {
    return FWXSTAT & FWX_STAT_S2;
}

static bool fwx_rx_ready(void) {
    return FWXSTAT & FWX_STAT_S1;
}

static void fwx_write_u8(uint8_t v) {
    FWXDATA = v;
    FWXCTRL |= FWX_CTRL_CLK;
}

static uint8_t fwx_read_u8(void) {
    FWXCTRL |= FWX_CTRL_CLK;
    return FWXDATA;
}

static void fwx_wait_tx_done(void) {
    while (!(FWXSTAT & FWX_STAT_S2))
        ;
}

static void fwx_wait_rx_ready(void) {
    while (!(FWXSTAT & FWX_STAT_S1))
        ;
}

#endif /* FWX_PUBLIC_H */
