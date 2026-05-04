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

#include <fwkes/bus.h>

#include <string.h>

static Bus g_bus;

#undef MEMORY_SIZE
#define MEMORY_SIZE 0x800

void setUp() {
    memset(&g_bus, 0, sizeof(g_bus));
    bus_init(&g_bus);
}

void tearDown() {

}

void test_ram_write_access(void) {
    uint8_t expected[MEMORY_SIZE] = { 0 };

    for (uint16_t i = 0; i < 0x800; ++i) {
        expected[i] = (uint8_t) i;
        bus_write(&g_bus, i, (uint8_t) i);
    }

    for (uint16_t i = 0; i < 0x800; ++i) {
        expected[i] = (uint8_t) i;
        bus_write(&g_bus, i, (uint8_t) i);
        TEST_ASSERT_EQUAL_HEX8(expected[i], i);
    }
}

void test_ram_read_access(void) {
    uint8_t expected[MEMORY_SIZE] = { 0 };
    uint8_t actual[MEMORY_SIZE] = { 0 };

    for (uint16_t i = 0; i < MEMORY_SIZE - 1; ++i) {
        expected[i] = (uint8_t) i;
        actual[i] = bus_read(&g_bus, i);
    }

    for (uint16_t i = 0; i < 0x800; ++i) {
        TEST_ASSERT_EQUAL_HEX8(expected[i], actual[i]);
    }
}

int main(void) {
    RUN_TEST(test_ram_read_access);
    RUN_TEST(test_ram_write_access);

    return 0;
}
