#include "platform.h"
#include "ff.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

#define CS 5
#define MOSI 3
#define MISO 4
#define SCK_SPI 2
#define SPI_PORT spi0
#define KILO 1000
#define MEGA 1000000
#define DUMMY 0xff

void sd_select(){
    gpio_put(CS, 0);
}

DWORD get_fattime(void) {
    return ((DWORD)(2024 - 1980) << 25) |
           ((DWORD)(1) << 21) |
           ((DWORD)(1) << 16) |
           ((DWORD)(0) << 11) |
           ((DWORD)(0) << 5) |
           ((DWORD)(0) >> 1);
}

void sd_deselect(){
    gpio_put(CS, 1);
    uint8_t dummy = DUMMY;
    spi_write_blocking(SPI_PORT, &dummy, 1);
}

uint8_t spi_write_byte(uint8_t data){
    uint8_t rx_data = 0;
    spi_write_read_blocking(SPI_PORT, &data, &rx_data, 1);
    return rx_data;
}

uint8_t spi_read_byte(){
    uint8_t data = 0;
    uint8_t dummy_tx = DUMMY;
    spi_write_read_blocking(SPI_PORT, &dummy_tx, &data, 1);
    return data;
}

void spi_read_bytes(uint8_t *buf, size_t len){
    uint8_t dummy_tx[len];
    for(size_t i = 0; i < len; i++) {
        dummy_tx[i] = DUMMY;
    }
    spi_write_read_blocking(SPI_PORT, dummy_tx, buf, len);
}

void spi_write_bytes(const uint8_t *buf, size_t len){
    spi_write_blocking(SPI_PORT, buf, len);
}

void platform_delay_ms(uint32_t ms){
    sleep_ms(ms);
}

void spi_set_low_speed(){
    spi_set_baudrate(SPI_PORT, 400*KILO);
}

void spi_set_high_speed(){
    spi_set_baudrate(SPI_PORT, 10*MEGA);
}

void spi_setup(){
    gpio_init(CS);
    gpio_set_dir(CS, GPIO_OUT);
    gpio_put(CS, 1);

    spi_init(SPI_PORT, 400 * 1000);
    spi_set_format(
        SPI_PORT,
        8,
        SPI_CPOL_0,
        SPI_CPHA_0,
        SPI_MSB_FIRST
    );

    gpio_set_function(MISO, GPIO_FUNC_SPI);
    gpio_set_function(SCK_SPI, GPIO_FUNC_SPI);
    gpio_set_function(MOSI, GPIO_FUNC_SPI);

    gpio_pull_up(MISO);
    sleep_ms(100);
}
