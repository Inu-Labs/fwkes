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

#include <fwkes/fs.h>

#ifdef BUILD_RP2350
#    include <diskio.h>
#    include <ff.h>
#    include <platform.h>
#    include <storage.h>
#elif __unix__
#    include <dirent.h>
#    include <unistd.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#elif _WIN32
#    include <direct.h>
#    include <io.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct FsInternal {
#ifdef BUILD_RP2350
    FATFS fat;
#endif
} FsInternal;

typedef struct FileHandle {
#ifdef BUILD_RP2350
    FIL *handle;
#else
    FILE *handle;
#endif
} FileHandle;

typedef struct DirHandle {
#if defined(BUILD_RP2350) || defined(__unix__)
    DIR *handle;
#elif _WIN32
    /* TODO */
#endif
} DirHandle;

#define PROPAGATE_IF_ERR(err) \
    if ((err) != FS_ERR_OK) return err;

#ifdef BUILD_RP2350
static BYTE posix_mode_to_fat_mode(const char *mode) {
    BYTE fmode = 0;

    char old_ch = '\0';
    for (const char *ptr = mode; *ptr; ++ptr) {
        char ch = *ptr;

        switch (ch) {
        case 'r':
            fmode = FA_READ;

            break;
        case 'w':
            fmode = FA_CREATE_ALWAYS | FA_WRITE;

            break;
        case 'a':
            fmode = FA_OPEN_APPEND | FA_WRITE;

            break;
        case '+':
            switch (old_ch) {
            case 'r':
                fmode = FA_READ | FA_WRITE;

                break;
            case 'w':
                fmode = FA_CREATE_ALWAYS | FA_WRITE | FA_READ;

                break;
            case 'a':
                fmode = FA_OPEN_APPEND | FA_WRITE | FA_READ;

                break;
            default:
                break;
            }

            break;
        case 'x':
            switch (old_ch) {
            case 'w':
                fmode = FA_CREATE_NEW | FA_WRITE;

                break;
            case '+':
                fmode = FA_CREATE_NEW | FA_WRITE | FA_READ;

                break;
            default:
                break;
            }
        default:
            break;
        }

        old_ch = ch;
    }

    return fmode;
}

static FsError get_error(FRESULT res, bool was_dir, bool known_to_exist) {
    switch (res) {
    case FR_OK:
        return FS_ERR_OK;
    case FR_LOCKED:
        return FS_ERR_BUSY;
    case FR_NO_FILE:
    case FR_NO_PATH:
        if (known_to_exist && was_dir) {
            return FS_ERR_NOT_FILE;
        } else if (known_to_exist && !was_dir) {
            return FS_ERR_NOT_DIR;
        } else {
            return FS_ERR_NO_EXIST;
        }
    case FR_INVALID_NAME:
        return FS_ERR_INVALID_PATH;
    case FR_DENIED:
        return FS_ERR_DENIED;
    case FR_NO_FILESYSTEM:
    case FR_NOT_ENABLED:
    case FR_INVALID_DRIVE:
    case FR_NOT_READY:
        return FS_ERR_UNAVAILABLE;
    default:
        return FS_ERR_INTERNAL;
    }
}
#else
static FsError get_error(int errno_code) {
    switch (errno_code) {
    case 0:
        return FS_ERR_OK;
    case EBUSY:
        return FS_ERR_BUSY;
    case ENOENT:
        return FS_ERR_NO_EXIST;
    case EISDIR:
        return FS_ERR_NOT_FILE;
    case ENOTDIR:
        return FS_ERR_NOT_DIR;
    case EACCES:
        return FS_ERR_DENIED;
    default:
        return FS_ERR_INTERNAL;
    }
}
#endif

FsError fs_init(Fs *self) {
#ifdef BUILD_RP2350
    self->internal = malloc(sizeof(FsInternal));

    if (!self->internal) {
        return FS_ERR_INTERNAL;
    }

    spi_setup();
    /* FIXME: get rid of fixed drive number */
    DSTATUS ds = disk_initialize(0);

    if (ds & STA_NOINIT) {
        return FS_ERR_INTERNAL;
    }

    FsInternal *internal = self->internal;
    FRESULT fr = f_mount(&internal->fat, "0:", 1);

    return get_error(fr, false, false);
#else
    self->internal = NULL;

    return FS_ERR_OK;
#endif
}

FsError fs_free(Fs *self) {
#ifdef BUILD_RP2350
    FRESULT fr = f_unmount("0:");
    free(self->internal);

    return get_error(fr, false, false);
#else
    (void) self;

    return FS_ERR_OK;
#endif
}

FsError fs_open(Fs *self, File *file, const char *path, const char *mode) {
    file->fs = self;

#ifdef BUILD_RP2350
    file->handle = malloc(sizeof(FIL));

    if (!file->handle) {
        return FS_ERR_INTERNAL;
    }

    BYTE fmode = posix_mode_to_fat_mode(mode);
    FRESULT res = f_open(file->handle, (const TCHAR *) path, fmode);

    return get_error(res, false, res != FR_NO_FILE);
#else
    errno = 0;
    file->handle = fopen(path, mode);

    return file->handle ? FS_ERR_OK : get_error(errno);
#endif
}

FsError fs_opendir(Fs *self, Dir *dir, const char *path) {
    dir->fs = self;

#ifdef BUILD_RP2350
    dir->handle = malloc(sizeof(DIR));

    if (!dir->handle) {
        return FS_ERR_INTERNAL;
    }

    FRESULT res = f_opendir(dir->handle, (const TCHAR *) path);

    return get_error(res, false, false);
#elif __unix__
    errno = 0;
    dir->handle = opendir(path);

    return dir->handle ? FS_ERR_OK : get_error(errno);
#elif _WIN32
    /* TODO */
#endif
}

FsError fs_create_file(Fs *self, const char *path) {
    static File file;
    FsError err = fs_open(self, &file, path, "w");

    if (err != FS_ERR_OK) {
        return err;
    }

    file_close(&file);

    return FS_ERR_OK;
}

FsError fs_cwd(Fs *self, const char *path) {
    (void) self;
#ifdef BUILD_RP2350
    FRESULT res = f_chdir((const TCHAR *) path);

    return get_error(res, false, false);
#else
    errno = 0;
#    ifdef _WIN32
    int res = _chdir(path);
#    else
    int res = chdir(path);
#    endif

    return res == 0 ? FS_ERR_OK : get_error(errno);
#endif
}

FsError fs_pwd(Fs *self, char *buf, size_t buf_size) {
    (void) self;

#ifdef BUILD_RP2350
    FRESULT res = f_getcwd((TCHAR *) buf, (UINT) buf_size);

    return get_error(res, false, false);
#else
    errno = 0;
#    ifdef _WIN32
    const char *path = _getcwd(buf, buf_size);
#    else
    const char *path = getcwd(buf, buf_size);
#    endif

    return path ? FS_ERR_OK : get_error(errno);
#endif
}

FsError fs_delete(Fs *self, const char *path) {
    (void) self;

#ifdef BUILD_RP2350
    FRESULT res = f_unlink((const TCHAR *) path);

    return get_error(res, false, false);
#else
    errno = 0;
#    ifdef _WIN32
    int res = _unlink(path);
#    else
    int res = unlink(path);
#    endif

    return res == 0 ? FS_ERR_OK : get_error(errno);
#endif
}

FsError fs_type(const char *path, FileType *out) {
#if BUILD_RP2350
    if (strcmp(path, "..") == 0) {
        *out = FILE_FOLDER;

        return FS_ERR_OK;
    }

    FILINFO info;
    FRESULT err = f_stat((const TCHAR *) path, &info);

    if (err != FR_OK) {
        return get_error(err, false, false);
    }

    if (info.fattrib & AM_DIR) {
        *out = FILE_FOLDER;
    } else {
        *out = FILE_REGULAR;
    }
#elif __unix__
    struct stat info;

    if (stat(path, &info) != 0) {
        return get_error(errno);
    }

    if (S_ISDIR(info.st_mode)) {
        *out = FILE_FOLDER;
    } else {
        *out = FILE_REGULAR;
    }
#elif _WIN32
    struct _stat info;

    if (_stat(path, &info) != 0) {
        return get_error(errno);
    }

    if (info.st_mode == _S_IFDIR) {
        *out = FILE_FOLDER;
    } else {
        *out = FILE_REGULAR;
    }
#endif

    return FS_ERR_OK;
}

FsError file_close(File *self) {
#ifdef BUILD_RP2350
    FRESULT res = f_close(self->handle);
    free(self->handle);

    return get_error(res, false, false);
#else
    errno = 0;
    int res = fclose(self->handle);

    return res == 0 ? FS_ERR_OK : get_error(errno);
#endif
}

FsError file_write(File *self, const void *data, size_t n) {
#ifdef BUILD_RP2350
    UINT written = 0;
    FRESULT res = f_write(self->handle, data, (UINT) n, &written);

    return get_error(res, false, false);
#else
    errno = 0;
    size_t written = fwrite(data, 1, n, self->handle);

    return written == n ? FS_ERR_OK : get_error(errno);
#endif
}

FsError file_read(File *self, void *buf, size_t n) {
#ifdef BUILD_RP2350
    UINT read_bytes = 0;
    FRESULT res = f_read(self->handle, buf, (UINT) n, &read_bytes);

    return get_error(res, false, false);
#else
    errno = 0;
    size_t read_bytes = fread(buf, 1, n, self->handle);

    return read_bytes == n ? FS_ERR_OK : get_error(errno);
#endif
}

FsError file_seek(File *self, long offset, FsOrigin origin) {
#ifdef BUILD_RP2350
    size_t size = 0;
    FsError err = file_size(self, &size);

    if (err != FS_ERR_OK) {
        return err;
    }

    FSIZE_t curr_off = f_tell((FIL *) self->handle);
    FSIZE_t final_offset = 0;

    switch (origin) {
    case FS_ORIGIN_BEG:
        final_offset = (FSIZE_t) offset;

        break;
    case FS_ORIGIN_CURR:
        final_offset = curr_off + (FSIZE_t) offset;

        break;
    case FS_ORIGIN_END:
        final_offset = (FSIZE_t) size + (FSIZE_t) offset;

        break;
    }

    FRESULT res = f_lseek(self->handle, final_offset);

    return get_error(res, false, false);
#else
    int whence;

    switch (origin) {
    case FS_ORIGIN_BEG:
        whence = SEEK_SET;

        break;
    case FS_ORIGIN_CURR:
        whence = SEEK_CUR;

        break;
    case FS_ORIGIN_END:
        whence = SEEK_END;

        break;
    };

    errno = 0;
    int res = fseek(self->handle, offset, whence);

    return res == 0 ? FS_ERR_OK : get_error(errno);
#endif
}

FsError file_tell(File *self, size_t *out) {
#ifdef BUILD_RP2350
    FSIZE_t pos = f_tell((FIL *) self->handle);
    *out = (size_t) pos;

    return FS_ERR_OK;
#else
    long pos = ftell(self->handle);

    if (pos == -1) {
        return get_error(errno);
    }

    *out = (size_t) pos;

    return FS_ERR_OK;
#endif
}

FsError file_size(File *self, size_t *out) {
#ifdef BUILD_RP2350
    FSIZE_t size = f_size((FIL *) self->handle);
    *out = (size_t) size;

    return FS_ERR_OK;
#else
    size_t old_pos = 0;
    FsError err = file_tell(self, &old_pos);
    PROPAGATE_IF_ERR(err);

    err = file_seek(self, 0, FS_ORIGIN_END);
    PROPAGATE_IF_ERR(err);

    err = file_tell(self, out);
    PROPAGATE_IF_ERR(err);

    return file_seek(self, (long) old_pos, FS_ORIGIN_BEG);
#endif
}

FsError dir_close(Dir *self) {
#ifdef BUILD_RP2350
    FRESULT res = f_closedir(self->handle);

    return get_error(res, false, false);
#elif __unix__
    errno = 0;
    closedir(self->handle);

    return FS_ERR_OK;
#elif _WIN32
    /* TODO */
#endif
}

FsError dir_read(Dir *self, DirEntry *entry) {
#ifdef BUILD_RP2350
    FILINFO fno;
    FRESULT res = f_readdir(self->handle, &fno);

    if (res != FR_OK) {
        return get_error(res, false, false);
    }

    if (fno.fname[0] != 0) {
        strncpy(entry->name, fno.fname, FILENAME_MAX_SIZE);
    } else {
        return FS_ERR_DIR_END;
    }

    return FS_ERR_OK;
#elif __unix__
    errno = 0;
    struct dirent *de = readdir(self->handle);

    if (de == NULL && errno != 0) {
        return get_error(errno);
    } else if (de == NULL) {
        return FS_ERR_DIR_END;
    } else {
        strncpy(entry->name, de->d_name, FILENAME_MAX_SIZE);
    }

    return FS_ERR_OK;
#elif _WIN32
    /* TODO */
#endif
}

FsError dir_rewind(Dir *self) {
#ifdef BUILD_RP2350
    FRESULT res = f_rewinddir(self->handle);

    return get_error(res, false, false);
#elif __unix__
    errno = 0;
    rewinddir(self->handle);

    return FS_ERR_OK;
#elif _WIN32
    /* TODO */
#endif
}
