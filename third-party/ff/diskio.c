/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2025        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h" /* Declarations FatFs MAI */
#include "ff.h"     /* Basic definitions of FatFs */

/* Example: Declarations of the platform and disk functions in the project */
#include "platform.h"
#include "storage.h"

/* Example: Mapping of physical drive number for each drive */
#define DEV_MMC 0 /* Map MMC/SD card to physical drive 0 */

/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status(
    BYTE pdrv /* Physical drive nmuber to identify the drive */
) {
    DSTATUS stat = STA_NOINIT;
    int result;
    stat = sd_get_status();

    return stat;
}

/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize(
    BYTE pdrv /* Physical drive nmuber to identify the drive */
) {
    DSTATUS stat = STA_NOINIT;
    int result;

    stat = sd_init();

    return stat;
}

/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read(
    BYTE pdrv,    /* Physical drive nmuber to identify the drive */
    BYTE *buff,   /* Data buffer to store read data */
    LBA_t sector, /* Start sector in LBA */
    UINT count    /* Number of sectors to read */
) {
    DRESULT res;
    int result;

    if (sd_read_sector(buff, sector, count)) {
        res = RES_OK;
    } else {
        res = RES_ERROR;
    }

    return res;
}

/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

DRESULT disk_write(
    BYTE pdrv,        /* Physical drive nmuber to identify the drive */
    const BYTE *buff, /* Data to be written */
    LBA_t sector,     /* Start sector in LBA */
    UINT count        /* Number of sectors to write */
) {
    DRESULT res;
    int result;

    if (!sd_write_sector(buff, sector, count))
        return RES_ERROR;
    res = RES_OK;
    // translate the reslut code here

    return res;
}

#endif

/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/

DRESULT disk_ioctl(
    BYTE pdrv, /* Physical drive nmuber (0..) */
    BYTE cmd,  /* Control code */
    void *buff /* Buffer to send/receive control data */
) {
    DRESULT res;
    int result;

    return res;
}
