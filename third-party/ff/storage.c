#include "storage.h"
#include "platform.h"
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

static uint8_t card_type = 0;
#define SD_INIT_RETRIES 6
#define SECTOR_SIZE 512
#define DATA_TOKEN        0xFE
#define DATA_ACCEPTED     0x05
#define SD_WRITE_TIMEOUT  50000

#define SD_CMD(x) (0x40 | (x))


uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg, uint8_t crc) {

    uint8_t r;
    spi_write_byte(0xFF); 

    spi_write_byte(cmd);       
    spi_write_byte(arg >> 24);
    spi_write_byte(arg >> 16);
    spi_write_byte(arg >> 8);
    spi_write_byte(arg);
    spi_write_byte(crc | 0x01);

    for (int i = 0; i < 10; i++) {
        r = spi_write_byte(0xFF);
        if ((r & 0x80) == 0) return r;
    }
    return r;
}
DSTATUS sd_init() {
    card_type = 0;
    spi_set_low_speed();
    
    sd_deselect();
    for(int i = 0; i < 10; i++) spi_write_byte(0xFF);

    sd_select();

    uint8_t r = 0xFF;
    for(int attempt = 0; attempt < 100; attempt++) {
        r = sd_send_cmd(SD_CMD(0), 0, 0x95);
        if(r == 0x01) break;
    }
    
    if(r != 0x01) {
        sd_deselect();
        return STA_NOINIT;
    }

    r = sd_send_cmd(SD_CMD(8), 0x1AA, 0x87);
    if(r == 0x01) {
        uint8_t buf[4];
        spi_read_bytes(buf, 4);
        if(buf[2] == 0x01 && buf[3] == 0xAA) {
            for(int i = 0; i < 1000; i++) {
                sd_send_cmd(SD_CMD(55), 0, 0);
                r = sd_send_cmd(SD_CMD(41), 1UL << 30, 0);
                if(r == 0) break;
                platform_delay_ms(1);
            }
            
            if (r == 0 && sd_send_cmd(SD_CMD(58), 0, 0) == 0) {
                uint8_t ocr[4];
                spi_read_bytes(ocr, 4);
                card_type = (ocr[0] & 0x40) ? 2 : 1;
            }
        }
    } else {
        card_type = 1;
    }

    sd_deselect();
    spi_write_byte(0xFF); // Dummy clock
    spi_set_high_speed();
    return (card_type > 0) ? 0 : STA_NOINIT;
}

bool sd_read_block(uint8_t *buff, uint32_t sector) {
    uint8_t token;
    uint32_t address = (card_type == 2) ? sector : (sector << 9);

    sd_select();

    if (sd_send_cmd(SD_CMD(17), address, 0) != 0x00) {
        sd_deselect();
        return false;
    }

    uint32_t timeout = 0xFFFF;
    do {
        token = spi_read_byte();
    } while (token == 0xFF && --timeout > 0);

    if (token != 0xFE) {
        sd_deselect();
        return false;
    }

    spi_read_bytes(buff, 512);
    spi_read_byte(); 
    spi_read_byte(); 

    sd_deselect();
    spi_write_byte(0xFF);
    
    return true;
}

bool sd_read_sector(uint8_t *buff, uint32_t sector, uint32_t count){
    for(uint32_t i = 0; i < count; i++){
        if(!sd_read_block(buff + i * 512, sector + i))
            return false;
    }
    return true;
}

DSTATUS sd_get_status(){
    return (card_type == 0) ? STA_NOINIT : 0;
}


bool sd_write_block(const uint8_t *buff, uint32_t sector)
{
    uint8_t resp;
    uint32_t timeout;

    sd_select();

    resp = sd_send_cmd(SD_CMD(24), sector, 0);
    if (resp != 0x00) {
        sd_deselect();
        return false;
    }

    spi_write_byte(0xFF);

    spi_write_byte(DATA_TOKEN);

    spi_write_bytes(buff, SECTOR_SIZE);

    spi_write_byte(0xFF);
    spi_write_byte(0xFF);

    resp = spi_read_byte();
    if ((resp & 0x1F) != DATA_ACCEPTED) {
        sd_deselect();
        return false;
    }

    timeout = SD_WRITE_TIMEOUT;
    while (spi_read_byte() == 0x00) {
        if (--timeout == 0) {
            sd_deselect();
            return false;
        }
    }

    sd_deselect();
    spi_write_byte(0xFF);

    return true;
}

bool sd_write_sector(const uint8_t *buff, uint32_t sector, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        if (!sd_write_block(buff + (i * SECTOR_SIZE), sector + i)) {
            return false;
        }
    }
    return true;
}
