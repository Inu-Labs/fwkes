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

#define BITP(bit) (1 << (bit))
#define BIT_CHECK(mask, bit) (((mask) & (bit)) == (bit))
#define BIT_SET(mask, bit) ((mask) |= (bit))
#define BIT_CLEAR(mask, bit) ((mask) &= ~(bit))
#define BIT_COPY(dst_mask, src_mask, bit)                                      \
    ({                                                                         \
        __typeof__(dst_mask) dst = (dst_mask);                                 \
        __typeof__(dst_mask) src = (src_mask);                                 \
        __typeof__(dst_mask) bitp = bit;                                       \
        (__typeof__(dst)) ((dst & ~bitp) | (src & bitp));                      \
    })
