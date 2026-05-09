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

/**
 * @file fs.h
 * @brief High-level filesystem API.
 *
 * This API provides high-level facilities for working with files and
 * directories.
 *
 * Specific features:
 *
 * - File streams
 * - Directory handles
 * - Basic I/O operations: write, read, seek
 * - Basic filesystem operations: remove, set current working directory, ...
 *
 * On the desktop backend, this API uses standard C functions + syscalls
 * for specific features. FatFS is used as underlying implementation on the
 * RP2350 backend.
 */

#include <stddef.h>

/**
 * @brief Filesystem state.
 */
typedef struct Fs {
    void *internal;
} Fs;

/**
 * @brief %File stream. Depending on the implementation, it can be buffered or
 * not.
 */
typedef struct File {
    Fs *fs;
    void *handle;
} File;

/**
 * @brief Directory handle.
 */
typedef struct Dir {
    Fs *fs;
    void *handle;
} Dir;

/**
 * @brief Filesystem error code.
 */
typedef enum FsError {
    FS_ERR_OK,           /**< Successful operation. */
    FS_ERR_INTERNAL,     /**< Internal or unknown error. */
    FS_ERR_UNAVAILABLE,  /**< Storage media is not available. */
    FS_ERR_BUSY,         /**< Target file is currently busy or is locked. */
    FS_ERR_NO_EXIST,     /**< Requested file does not exist. */
    FS_ERR_INVALID_PATH, /**< Provided filepath is ill-formed. */
    FS_ERR_DENIED,       /**< Access is denied or bad permissions. */
    FS_ERR_NOT_FILE,     /**< Target is not a regular file. */
    FS_ERR_NOT_DIR,      /**< Target is not a directory. */
    FS_ERR_DIR_END       /**< End of directory was reached. */
} FsError;

/**
 * @brief Seek base offset.
 */
typedef enum FsOrigin {
    FS_ORIGIN_BEG,  /**< Seek from the beginning of the file. */
    FS_ORIGIN_CURR, /**< Seek from the current position in the file. */
    FS_ORIGIN_END   /**< Seek from the end of the file. */
} FsOrigin;

/**
 * @brief Type of filesystem object.
 */
typedef enum FileType {
    FILE_REGULAR, /**< Target is a regular file. */
    FILE_FOLDER   /**< Target is a directory. */
} FileType;

/**
 * @def FILENAME_MAX_SIZE
 * @brief Max filename length including terminating character.
 */
#define FILENAME_MAX_SIZE 256

/**
 * @brief Directory file entry.
 */
typedef struct DirEntry {
    char name[FILENAME_MAX_SIZE]; /**< Filename. */
} DirEntry;

/**
 * @brief Initialize filesystem state.
 * @param self Fs instance.
 * @returns Error code of the last successful (or unsuccessful) operation.
 */
FsError fs_init(Fs *self);
/**
 * @brief Deinitialize filesystem state and free acquired resources.
 * @param self Fs instance.
 * @returns Error code of the last successful (or unsuccessful) operation.
 */
FsError fs_free(Fs *self);
/* clang-format off */
/**
 * @brief Open a file stream.
 *
 * The following basic open modes are supported:
 *
 * | Open mode | Description                   | Action if file already exists | Action if file does not exists |
 * |-----------|-------------------------------|-------------------------------|--------------------------------|
 * | `r`       | Open file for reading.        | Read from the beginning.      | Failure.                       |
 * | `w`       | Create a file for writing.    | Wipe out contents.            | Create a new file.             |
 * | `a`       | Append to file.               | Write to the end.             | Create a new file.             |
 * | `r+`      | Open a file for read/write    | Read from the beginning.      | Failure.                       |
 * | `w+`      | Create a file for read/write. | Wipe out contents.            | Create a new file.             |
 * | `a+`      | Open a file for read/write.   | Write to the end.             | Create a new file.             |
 *
 * Access flag `+` enables both input and write operations on the file.
 *
 * Access flag `x` forces the function to create file if it does not exist, and to fail if it exists.
 *
 * If none of the above flags is passed, it'll be ignored.
 *
 * %File is always opened in binary mode.
 *
 * @param self Fs instance.
 * @param[out] file Empty File instance.
 * @param path Filepath.
 * @param mode Open mode.
 * @returns Error code of the last successful (or unsuccessful) operation.
 */
/* clang-format on */
FsError fs_open(Fs *self, File *file, const char *path, const char *mode);
/**
 * @brief Open a directory handle.
 *
 * @param self Fs instance.
 * @param[out] dir Target directory handle.
 * @param path Filepath of the target directory.
 * @return Error code of the operation.
 */
FsError fs_opendir(Fs *self, Dir *dir, const char *path);
/**
 * @brief Create a regular file.
 *
 * @param self Fs instance.
 * @param path Filepath of the target directory.
 * @return Error code of the operation.
 */
FsError fs_create_file(Fs *self, const char *path);
/**
 * @brief Set the current working directory.
 *
 * @param self Fs instance.
 * @param path Filepath of the target directory.
 * @return Error code of the operation.
 */
FsError fs_cwd(Fs *self, const char *path);
/**
 * @brief Get the current working directory.
 *
 * @param self Fs instance.
 * @param[out] buf Buffer to store the path.
 * @param buf_size Maximal size of the buffer (including NUL).
 * @return Error code of the operation.
 */
FsError fs_pwd(Fs *self, char *buf, size_t buf_size);
/**
 * @brief Remove file or empty directory.
 *
 * @note Non-empty directories have to be removed recursively, i.e. one must
 * delete all files and then the directory itself.
 *
 * @param self Fs instance.
 * @param path Filepath of the target file.
 * @return Error code of the operation.
 */
FsError fs_delete(Fs *self, const char *path);
/**
 * @brief Get type of the object.
 *
 * Regular files are reported as FILE_REGULAR. Directories are FILE_FOLDER.
 *
 * @param self Fs instance.
 * @param[out] out Destination for storing file type.
 * @return Error code of the operation.
 */
FsError fs_type(const char *self, FileType *out);

/**
 * @brief Close file handle and free associated resources.
 *
 * @param self File instance.
 * @return Error code of the operation.
 */
FsError file_close(File *self);
/**
 * @brief Write data to file.
 *
 * When opening the file handle, the access flag "w" had to be specified. Also
 * the file must have write permissions.
 *
 * @param self File instance.
 * @param[in] buf Data to write.
 * @param n Number of bytes to write.
 * @return Error code of the operation.
 */
FsError file_write(File *self, const void *buf, size_t n);
/**
 * @brief Read data from file.
 *
 * When opening the file handle, the access flag "r" had to be specified. Also
 * the file must have read permissions.
 *
 * Actual size of buffer @p buf must be equal or greater than @p n.
 *
 * @param self File instance.
 * @param[out] buf Buffer to store data.
 * @param n Number of bytes to read.
 * @return Error code of the operation.
 */
FsError file_read(File *self, void *buf, size_t n);
/**
 * @brief Set cursor position in the file handle.
 *
 * Offset is relative to origin, which must be one of these values:
 *
 * - @ref FS_ORIGIN_BEG
 * - @ref FS_ORIGIN_CURR
 * - @ref FS_ORIGIN_END
 *
 * Positive offset moves forward, while negative moves backward. Zero offset
 * does nothing.
 *
 * @param self File instance.
 * @param offset Offset relative to @p origin.
 * @param origin Base for the offset.
 * @return Error code of the operation.
 */
FsError file_seek(File *self, long offset, FsOrigin origin);
/**
 * @brief Get the current position of the cursor in the file.
 *
 * @param self File instance.
 * @param[out] out Cursor position.
 * @return Error code of the operation.
 */
FsError file_tell(File *self, size_t *out);
/**
 * @brief Get the size in bytes of the file.
 *
 * @param self File instance.
 * @param[out] out File size.
 * @return Error code of the operation.
 */
FsError file_size(File *self, size_t *out);

/**
 * @brief Close directory handle and free associated resources.
 *
 * @param self File instance.
 * @return Error code of the operation.
 */
FsError dir_close(Dir *self);
/**
 * @brief Read the current entry in the directory and move to the next entry if
 * there is one.
 *
 * In case the last entry was already reached and read, this returns error code
 * @ref FS_ERR_DIR_END.
 *
 * @param self File instance.
 * @param[out] entry Directory entry.
 * @return Error code of the operation.
 */
FsError dir_read(Dir *self, DirEntry *entry);
/**
 * @brief Go to the first entry in the directory.
 *
 * @param self File instance.
 * @return Error code of the operation.
 */
FsError dir_rewind(Dir *self);
