#include "TMAG5170.hpp"

TMAG5170::TMAG5170(TMAG5170_version version) {
    this->version = version;

    return;
}

// Attach and initialise SPI on Pico for TMAG5170, default buadrate is 100kHz
void TMAG5170::attachSPI(spi_inst_t *spi, uint spi_sck_pin, uint spi_mosi_pin, uint spi_miso_pin, uint spi_cs_pin, uint buadrate = 1000000) {
    this->spi = spi;                    // set spi instance class variable
    spi_init(spi, buadrate);
    spi_set_format(spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);

    gpio_set_function(spi_sck_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi_mosi_pin, GPIO_FUNC_SPI);
    gpio_set_function(spi_miso_pin, GPIO_FUNC_SPI);

    this->spi_cs_pin = spi_cs_pin;      // set spi_cs_pin class variable
    gpio_init(spi_cs_pin);
    gpio_set_dir(spi_cs_pin, GPIO_OUT);
    gpio_put(spi_cs_pin, 1);            // default CS state high

    return;
}

// Generates CRC for SPI communication. The input data should contain no 1 in the last 4 bits.
uint32_t TMAG5170::generateCRC(uint32_t data) {
    uint32_t CRC = 0xf;     // least significant 4 bits are the shift register

    for(int i = 0; i < 32; i++) {
        uint32_t inv = ((data & 0x80000000) >> 31) ^ ((CRC & 0x8) >> 3);
            // XOR MSB of data and shift register
        uint32_t poly = (inv << 1) | inv;
            // polynomial = x^4 + x + 1
        uint32_t XORed = (CRC << 1) ^ poly;
        CRC = XORed & 0xf;
        data <<= 1;
    }    

    return CRC;
}

// Checks the CRC of the received frame. 
// Returns nonzero value if CRC is incorrect.
int TMAG5170::checkCRC(uint32_t recieved_frame) {
    uint32_t received_CRC_calculated = generateCRC(recieved_frame & 0xfffffff0);
        // clear the CRC bits and calculate the CRC
    if(received_CRC_calculated != recieved_frame & 0xf) {
        // calculated CRC does not match the actual received bits
        return 1;
    } else {
        return 0;
    }
}

// Exchanges an SPI frame with TMAG5170, the frame argument should include the CRC. 
// Returns the received frame. Does not check the CRC of the received frame.
uint32_t TMAG5170::exchangeFrame(uint32_t frame) {
    TMAG5170_SPI_frame send_frame, receive_frame;
    send_frame.data32 = __bswap32(frame);   // swap the bytes to match the order when viewed as array

    gpio_put(spi_cs_pin, 0);
    for(int i = 0; i < 4; i++) {
        spi_read_blocking(spi, send_frame.byte_arr[i], receive_frame.byte_arr + i, 1);
    }
    gpio_put(spi_cs_pin, 1);

    // update ERROR_STAT, containing the first received byte and the upper 4 bits of the last received byte.
    ERROR_STAT = (uint16_t)receive_frame.byte_arr[0];
    ERROR_STAT <<= 4;
    ERROR_STAT |= ((uint16_t)receive_frame.byte_arr[3] & 0xf0) >> 4;
    return __bswap32(receive_frame.data32);
}

// Reads the content of the register at the offset in the argument. This function reads the register until the CRC is correct.
// Set start_conversion_spi to initiate conversion when the CS line goes high.
uint16_t TMAG5170::readRegister(uint8_t offset, bool start_conversion_spi = false) {
    uint32_t sent_frame = ((uint32_t)offset | READ_REG) << 24;
    if(start_conversion_spi) sent_frame |= START_CONVERSION;
    sent_frame |= generateCRC(sent_frame);

    uint32_t received_frame;
    do {
        received_frame = exchangeFrame(sent_frame);
    } while(checkCRC(received_frame) != 0);

    received_frame >>= 8;               // extract the middle two bytes for the register content
    received_frame &= 0xffff;
    return (uint16_t)received_frame;
}

// Writes the register content into the register at the offset provided in the argument.
// Set start_conversion_spi to initiate conversion when the CS line goes high.
void TMAG5170::writeRegister(uint8_t offset, uint16_t register_content, bool start_conversion_spi) {
    uint32_t sent_frame = ((uint32_t)offset << 24) | ((uint32_t)register_content << 8);
    if(start_conversion_spi) sent_frame |= START_CONVERSION;
    sent_frame |= generateCRC(sent_frame);

    uint32_t received_frame;
    do {
        received_frame = exchangeFrame(sent_frame);
    } while(checkCRC(received_frame) != 0);

    return;
}