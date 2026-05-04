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

#include <fwkes/bus.h>
#include <fwkes/cpu.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static long get_file_size(FILE *file) {
    if (fseek(file, 0, SEEK_END) != 0) {
        return -1;
    }

    long pos = ftell(file);

    if (pos == -1) {
        return -1;
    }

    rewind(file);

    return pos;
}

static void *load_file(const char *path, size_t *size) {
    FILE *file = fopen(path, "rb");

    if (!file) {
        printf("error: cannot open file\n");

        return NULL;
    }

    long file_size = get_file_size(file);

    if (file_size == -1) {
        printf("error: cannot get file size\n");
        fclose(file);

        return NULL;
    }

    void *buf = malloc((size_t) file_size);

    if (!buf) {
        printf("error: cannot allocate file buffer\n");
        fclose(file);

        return NULL;
    }

    if (fread(buf, (size_t) file_size, 1, file) != 1 && ferror(file)) {
        printf("error: cannot read file\n");
        free(buf);
        fclose(file);

        return NULL;
    }

    *size = (size_t) file_size;

    fclose(file);

    return buf;
}

int main(void) {
    Bus bus = {0};
    Cpu cpu = {0};
    uint8_t memory[MEMORY_SIZE] = {0};

    size_t rom_size = 0;
    uint8_t *rom = load_file("nestest.nes", &rom_size);

    if (!rom) {
        return -1;
    }

    bus.memory = memory;
    memcpy(&memory[0x8000], &rom[0x0010], 0x4000);
    memcpy(&memory[0xc000], &rom[0x0010], 0x4000);
    memory[0xfffc] = 0x00;
    memory[0xfffd] = 0xc0;

    cpu.bus = &bus;
    cpu_init(&cpu, &bus);
    cpu.halt_on_brk = true;

    for (uint64_t i = 0; i < 15850 && !cpu.halt; ++i) {
        cpu_step(&cpu);
    }

    free(rom);

    return 0;
}
