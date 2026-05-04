#pragma once

#include <stdint.h>
#include <stddef.h>

void sd_select();
void sd_deselect();
uint8_t spi_write_byte(uint8_t data);
uint8_t spi_read_byte();
void spi_read_bytes(uint8_t *buf, size_t len);
void spi_write_bytes(const uint8_t *buf, size_t len);
void platform_delay_ms(uint32_t ms);
void spi_set_low_speed();
void spi_set_high_speed();
void spi_setup();

