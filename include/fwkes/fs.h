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

#include <stddef.h>

typedef struct Fs {
    void *internal;
} Fs;

typedef struct File {
    Fs *fs;
    void *handle;
} File;

typedef struct Dir {
    Fs *fs;
    void *handle;
} Dir;

typedef enum FsError {
    FS_ERR_OK,
    FS_ERR_INTERNAL,
    FS_ERR_UNAVAILABLE,
    FS_ERR_BUSY,
    FS_ERR_NO_EXIST,
    FS_ERR_INVALID_PATH,
    FS_ERR_DENIED,
    FS_ERR_NOT_FILE,
    FS_ERR_NOT_DIR,
    FS_ERR_DIR_END
} FsError;

typedef enum FsOrigin {
    FS_ORIGIN_BEG,
    FS_ORIGIN_CURR,
    FS_ORIGIN_END
} FsOrigin;

typedef enum FileType {
    FILE_REGULAR,
    FILE_FOLDER
} FileType;

#define FILENAME_MAX_SIZE 256

typedef struct DirEntry {
    char name[FILENAME_MAX_SIZE];
} DirEntry;

FsError fs_init(Fs *self);
FsError fs_free(Fs *self);
FsError fs_open(Fs *self, File *file, const char *path, const char *mode);
FsError fs_opendir(Fs *self, Dir *dir, const char *path);
FsError fs_create_file(Fs *self, const char *path);
FsError fs_cwd(Fs *self, const char *path);
FsError fs_pwd(Fs *self, char *buf, size_t buf_size);
FsError fs_delete(Fs *self, const char *path);
FsError fs_type(const char *self, FileType *out);

FsError file_close(File *self);
FsError file_write(File *self, const void *data, size_t size);
FsError file_read(File *self, void *buf, size_t buf_size);
FsError file_seek(File *self, long offset, FsOrigin origin);
FsError file_tell(File *self, size_t *out);
FsError file_size(File *self, size_t *out);

FsError dir_close(Dir *self);
FsError dir_read(Dir *self, DirEntry *entry);
FsError dir_rewind(Dir *self);
