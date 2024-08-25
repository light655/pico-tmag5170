#include "TMAG5170.hpp"

// Contructor, provide the version of the device as the argument
TMAG5170::TMAG5170(void) {

    return;
}

// Attach and initialise SPI on Pico for TMAG5170, default buadrate is 100kHz
void TMAG5170::attachSPI(spi_inst_t *spi, uint spi_sck_pin, uint spi_mosi_pin, uint spi_miso_pin, uint spi_cs_pin, uint buadrate) {
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
int TMAG5170::checkCRC(uint32_t received_frame) {
    uint32_t received_CRC_calculated = generateCRC(received_frame & 0xfffffff0);
        // clear the CRC bits and calculate the CRC
    if(received_CRC_calculated != (received_frame & 0xf)) {
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

// Reads the content of the register at the offset in the argument. This function attempts to read the register until the CRC is correct.
// Set start_conversion_spi to initiate conversion when the CS line goes high.
uint16_t TMAG5170::readRegister(uint8_t offset, bool start_conversion_spi) {
    uint32_t sent_frame = ((uint32_t)offset | READ_REG) << 24;
    if(start_conversion_spi) sent_frame |= START_CONVERSION;
    sent_frame |= generateCRC(sent_frame);

    uint32_t received_frame;
    do {
        received_frame = exchangeFrame(sent_frame);
    } while(checkCRC(received_frame) != 0);

    received_frame >>= 8;               // extract the middle two bytes for the register content
    received_frame &= 0xffff;
    uint16_t register_content = (uint16_t)received_frame;
    TMAG5170_registers[offset] = register_content;
    return register_content;
}

// Writes the register content stored in the array into the register at the offset provided in the argument. This function attempts to write the register until the returning CRC is correct.
// Set start_conversion_spi to initiate conversion when the CS line goes high.
void TMAG5170::writeRegister(uint8_t offset, bool start_conversion_spi) {
    uint16_t register_content = TMAG5170_registers[offset];
    uint32_t sent_frame = ((uint32_t)offset << 24) | ((uint32_t)register_content << 8);
    if(start_conversion_spi) sent_frame |= START_CONVERSION;
    sent_frame |= generateCRC(sent_frame);

    uint32_t received_frame;
    do {
        received_frame = exchangeFrame(sent_frame);
    } while(checkCRC(received_frame) != 0);

    return;
}

// Initialises TMAG5170, clears the CFG_REST bit and reads the version of the device.
// Returns the version of the device.
TMAG5170_version TMAG5170::init(void) {
    uint16_t AFE16;                     // reads the AFE_STATUS register twice, the MSB should be 0 by the second time.
    AFE16 = readRegister(AFE_STATUS);
    AFE16 = readRegister(AFE_STATUS);
    if(AFE16 & 0x8000) {
        version = ERROR;
        return ERROR;
    }

    uint16_t TEST16;
    TEST16 = readRegister(TEST_CONFIG);
    if(((TEST16 & 0x0030) >> 4) == 0x0) {
        version = A1;
    } else if(((TEST16 & 0x0030) >> 4) == 0x1) {
        version = A2;
    } else {
        version = ERROR;
    }
    return version;
}

// Sets the operating mode of the device. Use the macros to indicate operating mode.
void TMAG5170::setOperatingMode(uint16_t operating_mode) {
    TMAG5170_registers[DEVICE_CONFIG] &= ~OPERATING_MODE_MASK;
    TMAG5170_registers[DEVICE_CONFIG] |= operating_mode;
    writeRegister(DEVICE_CONFIG);

    return;
}

// Sets the number of averages per conversion of the device. Use the macros to indicate conversion average.
void TMAG5170::setConversionAverage(uint16_t conversion_average) {
    TMAG5170_registers[DEVICE_CONFIG] &= ~CONV_AVG_MASK;
    TMAG5170_registers[DEVICE_CONFIG] |= conversion_average;
    writeRegister(DEVICE_CONFIG);

    return;
}

// Enables angle calculation.
void TMAG5170::enableAngleCalculation(uint16_t angle_calculation_config) {
    TMAG5170_registers[SENSOR_CONFIG] &= ~ANGLE_EN_MASK;
    TMAG5170_registers[SENSOR_CONFIG] |= angle_calculation_config;
    writeRegister(SENSOR_CONFIG);

    return;
}

// Enables conversion on a selection of the three magnetic axes.
void TMAG5170::enableMagneticChannel(bool x_enable, bool y_enable, bool z_enable) {
    TMAG5170_registers[SENSOR_CONFIG] &= ~MAG_CH_EN_MASK;
    if(x_enable) {
        TMAG5170_registers[SENSOR_CONFIG] |= 0x0040;
    }
    if(y_enable) {
        TMAG5170_registers[SENSOR_CONFIG] |= 0x0080;
    }
    if(z_enable) {
        TMAG5170_registers[SENSOR_CONFIG] |= 0x0100;
    }
    writeRegister(SENSOR_CONFIG);

    return;
}

// Sets the magnetic field range of the respective axes.
void TMAG5170::setMagneticRange(uint16_t x_range, uint16_t y_range, uint16_t z_range) {
    TMAG5170_registers[SENSOR_CONFIG] &= ~(X_RANGE_MASK | Y_RANGE_MASK | Z_RANGE_MASK);
    TMAG5170_registers[SENSOR_CONFIG] |= x_range | y_range | z_range;
    writeRegister(SENSOR_CONFIG);

    return;
}

// Enables ALERT output to signal microcontroller conversion is finished.
void TMAG5170:: enableAlertOutput(bool enable) {
    TMAG5170_registers[ALERT_CONFIG] &= ~RSLT_ALRT_Asserted;
    if(enable) {
        TMAG5170_registers[ALERT_CONFIG] |= RSLT_ALRT_Asserted;
    }
    writeRegister(ALERT_CONFIG);

    return;
}

// Reads the conversion result of the magnetic field on the X axis.
// Returns the raw 16-bit value in the register.
uint16_t TMAG5170::readXRaw(bool start_conversion_spi) {
    return readRegister(X_CH_RESULT, start_conversion_spi);
}

// Reads the conversion result of the magnetic field on the Y axis.
// Returns the raw 16-bit value in the register.
uint16_t TMAG5170::readYRaw(bool start_conversion_spi) {
    return readRegister(Y_CH_RESULT, start_conversion_spi);
}

// Reads the conversion result of the magnetic field on the Z axis.
// Returns the raw 16-bit value in the register.
uint16_t TMAG5170::readZRaw(bool start_conversion_spi) {
    return readRegister(Z_CH_RESULT, start_conversion_spi);
}