#include "TMAG5170.hpp"

TMAG5170::TMAG5170(TMAG5170_version version) {
    this->version = version;

    return;
}

// Attach and initialise SPI on Pico for TMAG5170, default buadrate is 100kHz
void TMAG5170::attachSPI(spi_inst_t *spi, uint spi_sck_pin, uint spi_mosi_pin, uint spi_miso_pin, uint spi_cs_pin, uint buadrate = 1000000) {
    spi_init(spi, buadrate);
    spi_set_format(spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(spi_sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi_mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi_miso_pin, GPIO_FUNC_SPI);

    this->spi_cs_pin = spi_cs_pin;
    gpio_init(spi_cs_pin);
    gpio_set_dir(spi_cs_pin, GPIO_OUT);
    gpio_put(spi_cs_pin, 1);

    return;
}

// Generates CRC for SPI communication. The input data should contain no 1 in the last 4 bits.
uint32_t TMAG5170::generateCRC(uint32_t data) {
    uint32_t CRC = 0xf;

    for(int i = 0; i < 32; i++) {
        uint32_t inv = ((data & 0x80000000) >> 31) ^ ((CRC & 0x8) >> 3);
        uint32_t poly = (inv << 1) | inv;
        uint32_t XORed = (CRC << 1) ^ poly;
        CRC = XORed & 0xf;
        data <<= 1;
    }    

    return CRC;
}