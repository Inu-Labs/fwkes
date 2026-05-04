#ifndef FWX_COMMON_H
#define FWX_COMMON_H

/* MUST be a power of 2 */
#define FWX_MAX_DATA_SIZE 256

typedef enum FwxStatBits {
    FWX_STAT_S0 = (1 << 4),
    FWX_STAT_S1 = (1 << 5),
    FWX_STAT_S2 = (1 << 6)
} FwxStatBits;

typedef enum FwxCtrlBits {
    FWX_CTRL_SS = (1 << 0),
    FWX_CTRL_CLK = (1 << 1)
} FwxCtrlBits;

typedef enum FwxError {
    FWX_ERR_OK = 0x0,
    FWX_ERR_BAD_DATA = 0x1,
    FWX_ERR_SD_MISSING = 0x2,
    FWX_ERR_SD_IO_ERROR = 0x4,
    FWX_ERR_BAD_IDX = 0x5,
    FWX_ERR_NO_EXIST = 0x6,
    FWX_ERR_IS_FILE = 0x7,
    FWX_ERR_IS_DIR = 0x8
} FwxError;

typedef enum FwxCmdId {
    FWX_CMD_RESET = 0x00,
    FWX_CMD_SD_POLL = 0x01,
    FWX_CMD_SD_LOAD = 0x02,
    FWX_CMD_SD_LOAD_IDX = 0x03,
    FWX_CMD_SD_FILENAME_IDX = 0x04,
    FWX_CMD_SD_DELETE = 0x05,
    FWX_CMD_SD_DELETE_IDX = 0x06,
    FWX_CMD_SD_IS_DIR = 0x07,
    FWX_CMD_SD_IS_DIR_IDX = 0x08,
    FWX_CMD_SD_PWD = 0x09,
    FWX_CMD_SD_CWD = 0x0a,
    FWX_CMD_SD_FILE_COUNT = 0x0b,
    FWX_CMD_LED_ERROR = 0x0c,
    FWX_CMD_LED_GENERAL = 0x0d,
    FWX_CMD_BEEP = 0x0e
} FwxCmdId;

#endif /* FWX_COMMON_H */
