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
#include <fwkes/log.h>
#include <fwkes/trace.h>
#include <fwkes/disk.h>

#include <stdio.h>
#include <string.h>

#ifdef _WIN32
void *memmem(const void *haystack, size_t haystacklen,
             const void *needle, size_t needlelen)
{
    if (needlelen == 0)
        return (void *)haystack;

    const unsigned char *h = haystack;
    const unsigned char *n = needle;

    for (size_t i = 0; i + needlelen <= haystacklen; i++) {
        if (h[i] == n[0] && memcmp(h + i, n, needlelen) == 0)
            return (void *)(h + i);
    }
    return NULL;
}
#endif

int main(void) {
    static const char *roms[] = {
        "rom_singles/01-basics.nes",    "rom_singles/02-implied.nes",
        "rom_singles/03-immediate.nes", "rom_singles/04-zero_page.nes",
        "rom_singles/05-zp_xy.nes",     "rom_singles/06-absolute.nes",
        "rom_singles/07-abs_xy.nes",    "rom_singles/08-ind_x.nes",
        "rom_singles/09-ind_y.nes",     "rom_singles/10-branches.nes",
        "rom_singles/11-stack.nes",     "rom_singles/12-jmp_jsr.nes",
        "rom_singles/13-rts.nes",       "rom_singles/14-rti.nes",
        "rom_singles/15-brk.nes",       "rom_singles/16-special.nes"
    };

    trace_add_cat_filter(TRACE_PPU);
    log_add_cat_filter(LOG_PPU);
    log_add_cat_filter(LOG_BUS);

    Bus bus;
    bus_init(&bus);

    for (int i = 0; i < 16; ++i) {
        printf("\033[1m\033[34m::\033[39m Running %s...\033[0m\n", roms[i]);

        if (!bus_load_disk(&bus, roms[i])) {
            printf("cannot load '%s'\n", roms[i]);

            return -1;
        }

        Cpu *cpu = &bus.cpu;
        Disk *rom = &bus.disk;

        bus_reset(&bus);
        bus.cpu.disas = false;

        while (!cpu->halt) {
            cpu_step(cpu);

            if (memmem(&rom->prg_ram[4], 128, "Passed\0", 7)) {
                printf(" \033[1m\033[92mpass: \033[0m%s\n", roms[i]);

                break;
            } else if (memmem(&rom->prg_ram[4], 128, "Failed\0", 7)) {
                printf(" \033[1m\033[91mfail: \033[0m%s\n", roms[i]);

                char buf[1024] = {0};
                strcpy(buf, (const char *) &rom->prg_ram[4]);

                char *tok = strtok(buf, " \n");
                int tok_n = 0;

                while (tok) {
                    const char *rom_name = strchr(roms[i], '/') + 1;
                    if (strncmp(rom_name, tok, strlen(rom_name) - 4) == 0) {
                        break;
                    }

                    if (tok_n == 0) {
                        putchar(' ');
                    }

                    fputs(tok, stdout);

                    tok = strtok(NULL, " \n");
                    ++tok_n;

                    if (tok_n == 3) {
                        putchar('\n');
                        tok_n = 0;
                    } else {
                        putchar(' ');
                    }
                }

                break;
            }
        }
    }

    bus_deinit(&bus);

    return 0;
}
