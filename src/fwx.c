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

#include <fwkes/fwx/private.h>

#include <fwkes/bus.h>
#include <fwkes/disk.h>
#include <fwkes/trace.h>

#ifdef BUILD_RP2350
#    include <pico/stdlib.h>
#
#    define SD_CHECKER_FILENAME ".sd_checker"
#    define SD_CHECKER_PATH "/" SD_CHECKER_FILENAME
#endif

#include <stdlib.h>
#include <string.h>

#define PROPAGATE_IF_FS_ERR(err)                                               \
    if ((err) != FS_ERR_OK)                                                    \
        return err;

static const FwxError g_fs_err_to_fwx[] = {
    [FS_ERR_OK] = FWX_ERR_OK,
    [FS_ERR_INTERNAL] = FWX_ERR_INTERNAL,
    [FS_ERR_UNAVAILABLE] = FWX_ERR_INTERNAL,
    [FS_ERR_BUSY] = FWX_ERR_INTERNAL,
    [FS_ERR_NO_EXIST] = FWX_ERR_NO_EXIST,
    [FS_ERR_INVALID_PATH] = FWX_ERR_NO_EXIST,
    [FS_ERR_DENIED] = FWX_ERR_INTERNAL,
    [FS_ERR_NOT_FILE] = FWX_ERR_IS_DIR,
    [FS_ERR_NOT_DIR] = FWX_ERR_IS_FILE,
    [FS_ERR_DIR_END] = FWX_ERR_INTERNAL,
};

static const unsigned g_cmd_size[] = {
    [FWX_CMD_RESET] = 1,         [FWX_CMD_SD_POLL] = 1,
    [FWX_CMD_SD_INIT] = 1,       [FWX_CMD_SD_LOAD] = 2,
    [FWX_CMD_SD_FILENAME] = 2,   [FWX_CMD_SD_IS_DIR] = 2,
    [FWX_CMD_SD_PWD] = 1,        [FWX_CMD_SD_CWD] = 2,
    [FWX_CMD_SD_FILE_COUNT] = 1, [FWX_CMD_LED_ERROR] = 2,
    [FWX_CMD_LED_GENERAL] = 2,
    [FWX_CMD_SD_GET_ENTRY] = 2
};

static void reset_bufs(Fwx *self) {
    fwx_buf_reset(&self->tx);
    fwx_buf_reset(&self->rx);
}

static void clear_error(Fwx *self) { self->regs.STAT &= ~0xf; }

static void reset_state(Fwx *self) {
    self->regs.STAT |= FWX_STAT_S0;
    self->regs.STAT &= ~(FWX_STAT_S1 | FWX_STAT_S2);
    reset_bufs(self);
    self->curr_cmd = 0;
    clear_error(self);
}

static FwxError cmd_reset(Fwx *self);
static FwxError cmd_sd_poll(Fwx *self);
static FwxError cmd_sd_init(Fwx *self);
static FwxError cmd_sd_load(Fwx *self);
static FwxError cmd_sd_filename(Fwx *self);
static FwxError cmd_sd_is_dir(Fwx *self);
static FwxError cmd_sd_pwd(Fwx *self);
static FwxError cmd_sd_cwd(Fwx *self);
static FwxError cmd_sd_file_count(Fwx *self);
static FwxError cmd_led_error(Fwx *self);
static FwxError cmd_led_general(Fwx *self);

static FwxError cmd_sd_get_entry(Fwx *self) {
    unsigned idx = fwx_buf_get(&self->rx);

    if (idx >= self->file_count) {
        return FWX_ERR_BAD_IDX;
    }

    const char *path = self->files[idx];
    unsigned len = (unsigned) strlen(path);

    bool is_dir = (len > 0 && path[len - 1] == '/');

    fwx_buf_put(&self->tx, is_dir ? 1 : 0);

    if (is_dir && len > 0) {
        len--;
    }

    for (unsigned i = 0; i < len; i++) {
        fwx_buf_put(&self->tx, (uint8_t) path[i]);
    }

    fwx_buf_put(&self->tx, 0);

    return FWX_ERR_OK;
}

static FwxError (*g_cmd_table[])(Fwx *self) = {
    [FWX_CMD_RESET] = cmd_reset,
    [FWX_CMD_SD_POLL] = cmd_sd_poll,
    [FWX_CMD_SD_INIT] = cmd_sd_init,
    [FWX_CMD_SD_LOAD] = cmd_sd_load,
    [FWX_CMD_SD_FILENAME] = cmd_sd_filename,
    [FWX_CMD_SD_IS_DIR] = cmd_sd_is_dir,
    [FWX_CMD_SD_PWD] = cmd_sd_pwd,
    [FWX_CMD_SD_CWD] = cmd_sd_cwd,
    [FWX_CMD_SD_FILE_COUNT] = cmd_sd_file_count,
    [FWX_CMD_LED_ERROR] = cmd_led_error,
    [FWX_CMD_LED_GENERAL] = cmd_led_general,
    [FWX_CMD_SD_GET_ENTRY] = cmd_sd_get_entry
};

void fwx_execute_cmd(Fwx *self, FwxCmdId id) {

    if (id > sizeof(g_cmd_table)) {
        return;
    }

    FwxError err = g_cmd_table[id](self);
    fwx_set_error(self, err);
}

static void handle_clk(Fwx *self) {
    if (!(self->regs.STAT & FWX_STAT_S1)) {
        fwx_buf_put(&self->rx, self->regs.DATA);

        trace_fwx(
            TRACE_RX_DATA, "rx.data[%u] = 0x%02x (%c)\n", self->rx.pos - 1,
            self->regs.DATA, (char) self->regs.DATA
        );

        if (self->rx.size == 1) {
            FwxCmdId cmd = self->rx.data[0];

            if (cmd >= sizeof(g_cmd_table) / sizeof(g_cmd_table[0])) {
                fwx_buf_reset(&self->rx);
                fwx_buf_rewind(&self->tx);

                return;
            }

            self->curr_cmd = cmd;
            self->rx.expected_size = g_cmd_size[cmd];
        }

        if (self->rx.size == self->rx.expected_size) {
            self->regs.STAT |= FWX_STAT_S2;
            self->rx.pos = 1;

            fwx_execute_cmd(self, self->curr_cmd);

            fwx_buf_reset(&self->rx);
            fwx_buf_rewind(&self->tx);

            if (self->tx.size > 0) {
                self->regs.STAT |= FWX_STAT_S1;
            }
        }
    } else {
        if (self->tx.pos < self->tx.size) {
            uint8_t data = fwx_buf_get(&self->tx);
            self->regs.DATA = data;

            trace_fwx(
                TRACE_TX_DATA, "tx.data[%u] = 0x%02x (%c)\n", self->tx.pos - 1,
                self->regs.DATA, (char) self->regs.DATA
            );
        }

        if (self->tx.pos >= self->tx.size) {
            self->regs.STAT &= ~FWX_STAT_S1;
            fwx_buf_reset(&self->tx);
        }
    }

    self->regs.CTRL &= ~FWX_CTRL_CLK;
}

static FwxError cmd_reset(Fwx *self) {
    reset_state(self);

    self->file_count = 0;

#ifdef BUILD_RP2350
    dir_close(&self->cwd);

    char cwd[256];

    fs_cwd(self->fs, "/");
    fs_pwd(self->fs, cwd, 256);
    fs_opendir(self->fs, &self->cwd, cwd);
#endif

    return FWX_ERR_OK;
}

static FwxError cmd_led_error(Fwx *self) {
    bool on = fwx_buf_get(&self->rx);

#ifdef BUILD_RP2350
    /* TODO: make gpio for led */
#else
    printf(on ? "LED ERROR ON\n" : "LED ERROR OFF \n");
#endif

    return FWX_ERR_OK;
}

static FwxError cmd_led_general(Fwx *self) {
    bool on = fwx_buf_get(&self->rx);

#ifdef BUILD_RP2350
    /* TODO: make gpio for led */
#else
    printf(on ? "LED GENERAL ON\n" : "LED GENERAL OFF \n");
#endif

    return FWX_ERR_OK;
}

static FwxError cmd_sd_poll(Fwx *self) {
    (void) self;

#if BUILD_RP2350
    File file;

    if (fs_open(self->fs, &file, ".sd_checker", "r") != FS_ERR_OK) {
        return FWX_ERR_BAD_SD;
    }

    file_close(&file);
#endif

    return FWX_ERR_OK;
}

static FsError open_cwd(Fwx *self) {
    FsError err;
    char cwd[256] = {0};

    err = fs_pwd(self->fs, cwd, 255);
    PROPAGATE_IF_FS_ERR(err);

    return fs_opendir(self->fs, &self->cwd, cwd);
}

static FsError load_rom(Fwx *self, const char *path) {
    FileType type;

    FsError err = fs_type(path, &type);
    PROPAGATE_IF_FS_ERR(err);

    if (type == FILE_FOLDER) {
        err = fs_cwd(self->fs, path);
        PROPAGATE_IF_FS_ERR(err);

        err = open_cwd(self);
        PROPAGATE_IF_FS_ERR(err);

        err = fwx_update_file_list(self);
        PROPAGATE_IF_FS_ERR(err);
    } else {
        BusEvent ev = {
            .id = BUS_EVENT_LOAD_ROM,
            .load_rom = {.path = path},
        };

        bus_add_event(self->bus, &ev);
    }

    return FS_ERR_OK;
}

static FwxError cmd_sd_load(Fwx *self) {
    uint8_t idx = fwx_buf_get(&self->rx);

    if (idx >= self->file_count) {
        return FWX_ERR_BAD_IDX;
    }

    const char *path = self->files[idx];
    FsError err = load_rom(self, path);

    return g_fs_err_to_fwx[err];
}

static FwxError cmd_sd_filename(Fwx *self) {
    unsigned idx = fwx_buf_get(&self->rx);

    if (idx >= self->file_count) {
        return FWX_ERR_BAD_IDX;
    }

    strncpy(
        (char *) &self->tx.data[0], self->files[idx], FWX_MAX_DATA_SIZE - 1
    );

    unsigned size = (unsigned) strlen((char *) self->tx.data) + 1;
    self->tx.expected_size = size;
    self->tx.size = size;

    return FWX_ERR_OK;
}

static FwxError cmd_sd_is_dir(Fwx *self) {
    unsigned idx = fwx_buf_get(&self->rx);

    if (idx >= self->file_count) {
        return FWX_ERR_BAD_IDX;
    }

    const char *path = self->files[idx];
    FileType type;

    FsError err = fs_type(path, &type);

    if (err != FS_ERR_OK) {
        return g_fs_err_to_fwx[err];
    }

    if (type == FILE_FOLDER) {
        fwx_buf_put(&self->tx, 1);
    } else {
        fwx_buf_put(&self->tx, 0);
    }

    return FWX_ERR_OK;
}

static FwxError cmd_sd_pwd(Fwx *self) {
    FsError err =
        fs_pwd(self->fs, (char *) fwx_buf_ref(&self->tx), FWX_MAX_DATA_SIZE);

    if (err != FS_ERR_OK) {
        return g_fs_err_to_fwx[err];
    }

    unsigned size = (unsigned) strlen((char *) self->tx.data) + 1;
    self->tx.expected_size = size;
    self->tx.size = size;

    return FWX_ERR_OK;
}

static FwxError cmd_sd_cwd(Fwx *self) {
    char *path = (char *) fwx_buf_ref(&self->rx);
    self->rx.data[FWX_MAX_DATA_SIZE - 1] = '\0';

    FsError err = fs_cwd(self->fs, path);

    if (err != FS_ERR_OK) {
        return g_fs_err_to_fwx[err];
    }

    err = fwx_update_file_list(self);

    return g_fs_err_to_fwx[err];
}

static FwxError cmd_sd_file_count(Fwx *self) {
    fwx_buf_put(&self->tx, self->file_count & 0xff);

    return FWX_ERR_OK;
}

static FwxError cmd_sd_init(Fwx *self) {
    /* TODO: implement */
    (void) self;

    return FWX_ERR_OK;
}

void fwx_buf_reset(FwxBuffer *self) {
    memset(self->data, 0, sizeof(self->data));
    self->size = 0;
    self->expected_size = FWX_MAX_DATA_SIZE;
    self->pos = 0;
}

void fwx_buf_put(FwxBuffer *self, uint8_t data) {
    if (self->size < self->expected_size) {
        self->data[self->pos++] = data;
        ++self->size;
    }
}

uint8_t fwx_buf_get(FwxBuffer *self) { return self->data[self->pos++]; }

bool fwx_init(Fwx *self, Fs *fs, Bus *bus) {
    self->bus = bus;
    self->regs.STAT = 0x00;
    self->regs.CTRL = 0x00;
    self->regs.DATA = 0x00;

    self->file_count = 0;
    self->fs = fs;

    reset_state(self);

#if BUILD_RP2350
    if (fs_create_file(self->fs, SD_CHECKER_PATH) != FS_ERR_OK) {
        return false;
    }
#endif

    if (open_cwd(self) != FS_ERR_OK) {
        return false;
    }

    return fwx_update_file_list(self) == FS_ERR_OK;
}

void fwx_deinit(Fwx *self) {
    dir_close(&self->cwd);

#if BUILD_RP2350
    fs_delete(self->fs, SD_CHECKER_PATH);
#endif
}

bool fwx_reset(Fwx *self) {
    dir_close(&self->cwd);
    memset(&self->regs, 0, sizeof(self->regs));
    fwx_buf_reset(&self->tx);
    fwx_buf_reset(&self->rx);
    disk_unload(&self->disk);
    self->file_count = 0;
    memset(self->files, 0, sizeof(self->files));

    if (open_cwd(self) != FS_ERR_OK) {
        return false;
    }

    return fwx_update_file_list(self) == FS_ERR_OK;
}

static int qsort_strcmp(const void *a, const void *b) { return strcmp(a, b); }

FsError fwx_update_file_list(Fwx *self) {
    self->file_count = 0;

    for (int i = 0; i < 256; ++i) {
        memset(self->files[i], 0, 256);
    }

    dir_rewind(&self->cwd);

    DirEntry de;

#ifdef BUILD_RP2350
    /* FatFs' readdir does not return '..' in contrast to Linux' readdir. */
    strncpy(self->files[0], "../", FILENAME_MAX_SIZE);
    ++self->file_count;
#endif

    while (self->file_count < 255) {
        FsError err = dir_read(&self->cwd, &de);

        if (err == FS_ERR_DIR_END) {
            break;
        } else if (err != FS_ERR_OK) {
            return err;
        }

        if (strcmp(de.name, ".") == 0) {
            continue;
        }

#if BUILD_RP2350
        if (strcmp(de.name, ".sd_checker") != 0) {
#endif
            FileType type;
            if (fs_type(de.name, &type) == FS_ERR_OK && type == FILE_FOLDER) {
                snprintf(
                    self->files[self->file_count], FILENAME_MAX_SIZE, "%s/",
                    de.name
                );
            } else {
                strncpy(
                    self->files[self->file_count], de.name, FILENAME_MAX_SIZE
                );
            }

            ++self->file_count;
#if BUILD_RP2350
        }
#endif
    }

    qsort(self->files, self->file_count, 256, qsort_strcmp);

    return FS_ERR_OK;
}

void fwx_set_error(Fwx *self, FwxError err) {
    clear_error(self);
    self->regs.STAT |= err & 0xf;
}

void fwx_write(Fwx *self, uint16_t addr, uint8_t data) {
    switch (addr) {
    case FWX_STAT:
        break;
    case FWX_CTRL:
        fwx_write_ctrl(self, data);

        break;
    case FWX_DATA:
        self->regs.DATA = data;

        break;
    default:
        break;
    }
}

uint8_t fwx_read(Fwx *self, uint16_t addr) {
    uint8_t data;

    switch (addr) {
    case FWX_STAT:
        data = fwx_read_stat(self);
        trace_fwx(TRACE_REG_READ, "STAT = 0x%02x", data);

        break;
    case FWX_CTRL:
        data = self->regs.CTRL;
        trace_fwx(TRACE_REG_READ, "CTRL = 0x%02x", data);

        break;
    case FWX_DATA:
        data = self->regs.DATA;
        trace_fwx(TRACE_REG_READ, "DATA = 0x%02x", data);

        break;
    default:
        return 0x00;
    }

    return data;
}

uint8_t fwx_peek(const Fwx *self, uint16_t addr) {
    switch (addr) {
    case FWX_STAT:
        return self->regs.STAT;
    case FWX_CTRL:
        return self->regs.CTRL;
    case FWX_DATA:
        return self->regs.DATA;
    default:
        return 0x00;
    }
}

uint8_t fwx_read_stat(Fwx *self) {
    uint8_t old = self->regs.STAT;

    if (self->regs.STAT & FWX_STAT_S2) {
        self->regs.STAT &= ~FWX_STAT_S2;
    }

    return old;
}

void fwx_write_ctrl(Fwx *self, uint8_t value) {
    uint8_t prev = self->regs.CTRL;
    self->regs.CTRL = value;

    /* START condition */
    if (!(prev & FWX_CTRL_SS) && (value & FWX_CTRL_SS)) {
        trace_fwx(TRACE_START_COND, "");
        reset_state(self);

        return;
    }

    /* STOP condition */
    if ((prev & FWX_CTRL_SS) && !(value & FWX_CTRL_SS)) {
        trace_fwx(TRACE_STOP_COND, "");
        self->regs.STAT &= ~(FWX_STAT_S0 | FWX_STAT_S1 | FWX_STAT_S2);
        clear_error(self);

        return;
    }

    /* CLK edge clock */
    if ((self->regs.CTRL & FWX_CTRL_SS) && (value & FWX_CTRL_CLK)) {
        trace_fwx(TRACE_CLK_EDGE, "");
        handle_clk(self);
    }
}
