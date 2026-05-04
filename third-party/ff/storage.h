#pragma once

#include "diskio.h"
#include "storage.h"
#include "platform.h"
#include <stdbool.h>
#include <stdint.h>

DSTATUS sd_init();
uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc);
bool sd_read_block(uint8_t *buff, uint32_t sector);
bool sd_read_sector(uint8_t *buff, uint32_t sector, uint32_t count);
bool sd_write_block(const uint8_t *buff, uint32_t sector);
bool sd_write_sector(const uint8_t *buff, uint32_t sector, uint32_t count);
DSTATUS sd_get_status();
