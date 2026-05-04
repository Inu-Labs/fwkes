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

#include <unity.h>

#include <fwkes/fs.h>
#include <fwkes/fwx_internal.h>

Fwx g_fwx;
Fs g_fs;

void setUp(void) {}
void tearDown(void) {}

static void test_initial_state(void) {
    TEST_ASSERT_EQUAL_UINT8(0x00, g_fwx.regs.STAT);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_fwx.regs.CTRL);
    TEST_ASSERT_EQUAL_UINT8(0x00, g_fwx.regs.DATA);
}

static void test_start_condition(void) {
    fwx_write_ctrl(&g_fwx, 0x01);
}

int main(void) {
    fs_init(&g_fs);

    RUN_TEST(test_initial_state);

    fs_free(&g_fs);

    return 0;
}
