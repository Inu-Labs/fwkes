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

#ifndef FWX_PUBLIC_H
#define FWX_PUBLIC_H

#include "common.h"

#define FWXSTAT *((volatile unsigned char *) 0x4018)
#define FWXCTRL *((volatile unsigned char *) 0x4019)
#define FWXDATA *((volatile unsigned char *) 0x401a)

#define fwx_start() (FWXCTRL = FWX_CTRL_SS)
#define fwx_stop() (FWXCTRL = 0)

#define fwx_tx_done() (FWXSTAT & FWX_STAT_S2)
#define fwx_rx_ready() (FWXSTAT & FWX_STAT_S1)

#define fwx_write_u8(data) (FWXDATA = data, FWXCTRL |= FWX_CTRL_CLK)
#define fwx_read_u8() (FWXCTRL |= FWX_CTRL_CLK, FWXDATA)

#define fwx_wait_tx_done() while (!fwx_tx_done())
#define fwx_wait_rx_ready() while (!fwx_rx_ready())

#define fwx_last_error() (FWXSTAT & 0x0f)

#endif /* FWX_PUBLIC_H */
