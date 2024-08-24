#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

#include "TMAG5170.hpp"
#define TMAG_ALERT 17
#define SPI_CS 21

#define PRINT_DEBUG

typedef union {
    uint8_t byte_arr[4];
    uint32_t data32;
} SPI_frame;

void initTMAG(void);
int sendTMAG(uint32_t);
SPI_frame readRegister(uint8_t);
uint32_t generateCRC(uint32_t);

void alertCallback(uint, uint32_t);

int main(void) {
    gpio_init(SPI_CS);
    gpio_set_dir(SPI_CS, true);    // set as output
    gpio_put(SPI_CS, 1);

    stdio_init_all();
    sleep_ms(5000);
    printf("Hello world!\n");

    spi_init(spi0, 100 * 1000);
    spi_set_format(spi0, 8, 0, 0, SPI_MSB_FIRST);
    gpio_set_function(18, GPIO_FUNC_SPI);
    gpio_set_function(19, GPIO_FUNC_SPI);
    gpio_set_function(20, GPIO_FUNC_SPI);

    // gpio_put(SPI_CS, 0);
    // uint8_t tmp_data[4] = {0x0f, 0x00, 0x04, 0x07};
    // spi_write_blocking(spi0, tmp_data, 4);
    // gpio_put(SPI_CS, 1);

    readRegister(0xD);
    readRegister(0xD);

    gpio_set_irq_enabled_with_callback(TMAG_ALERT, GPIO_IRQ_EDGE_FALL, true, &alertCallback);
    initTMAG();
    sleep_ms(1000);
    alertCallback(0, 0);

    while(true) {
        tight_loop_contents();
    }

    return 0;
}

void initTMAG(void) {
    uint16_t data16;
    uint32_t data;

    for(uint32_t offset = 0x15; offset <= 0x15; offset--) {
        switch(offset) {
        case DEVICE_CONFIG:
            data16 = CONV_AVG_32x | MAG_TEMPCO_0pd | OPERATING_MODE_ActiveMeasureMode | T_CH_EN_TempChannelDisabled | T_RATE_sameAsOtherSensors | T_HLT_EN_tempLimitCheckOff | TEMP_COMP_EN_TempCompensationDisenabled;
            data = (uint32_t)offset << 24 | (uint32_t)data16 << 8;   // start conversion
            sendTMAG(data);
            break;
        case SENSOR_CONFIG:
            data16 = ANGLE_EN_NoAngleCalculation | SLEEPTIME_1ms | MAG_CH_EN_XYZenabled | Z_RANGE_300mT | Y_RANGE_300mT | X_RANGE_300mT;
            data = (uint32_t)offset << 24 | (uint32_t)data16 << 8;
            sendTMAG(data);
            break;
        case SYSTEM_CONFIG:
            data16 = DIAG_SEL_AllDataPath | TRIGGER_MODE_SPI | DATA_TYPE_32bit | DIAG_EN_AFEdiagnosticsDisabled | Z_HLT_EN_ZaxisLimitCheckoff | Y_HLT_EN_YaxisLimitCheckoff | X_HLT_EN_XaxisLimitCheckoff;
            data = (uint32_t)offset << 24 | (uint32_t)data16 << 8;
            sendTMAG(data);
            break;
        case ALERT_CONFIG:
            data16 = RSLT_ALRT_Asserted;
            data = (uint32_t)offset << 24 | (uint32_t)data16 << 8;
            sendTMAG(data);
            break;
        case MAG_GAIN_CONFIG:
            data16 = GAIN_SELECTION_NoAxis;
            data = (uint32_t)offset << 24 | (uint32_t)data16 << 8;
            sendTMAG(data);
            break;
        default:
            break;
        }
        readRegister(offset);
    }

    return;
}

int sendTMAG(uint32_t data) {
    data |= generateCRC(data);
    SPI_frame send_frame, receive_frame;
    send_frame.data32 = __builtin_bswap32(data);    // convert endianness

    gpio_put(SPI_CS, 0);
    for(int i = 0; i < 4; i++) {
        spi_read_blocking(spi0, send_frame.byte_arr[i], receive_frame.byte_arr + i, 1);
    }
    gpio_put(SPI_CS, 1);

    uint32_t received_CRC = generateCRC(__builtin_bswap32(receive_frame.data32) & 0xfffffff0);
#ifdef PRINT_DEBUG
    printf("Frame sent: %.8X\n", __builtin_bswap32(send_frame.data32));
    printf("Frame received: %.8X\n", __builtin_bswap32(receive_frame.data32));
#endif
    if(received_CRC != __builtin_bswap32(receive_frame.data32) & 0xf) {
        printf("Send CRC error! %X\n", received_CRC);
        return 1;
    } else {
        return 0;
    }
}

uint32_t generateCRC(uint32_t data) {
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

SPI_frame readRegister(uint8_t offset) {
    SPI_frame send_frame, receive_frame;
    send_frame.byte_arr[0] = offset | READ_REG;
    send_frame.byte_arr[1] = 0x0;
    send_frame.byte_arr[2] = 0x0;
    send_frame.byte_arr[3] = 0x0;
    uint32_t CRC = generateCRC(__builtin_bswap32(send_frame.data32));
    send_frame.byte_arr[3] |= (uint8_t)CRC;
    
    gpio_put(SPI_CS, 0);
    for(int i = 0; i < 4; i++) {
        spi_read_blocking(spi0, send_frame.byte_arr[i], receive_frame.byte_arr + i, 1);
    }
    gpio_put(SPI_CS, 1);
#ifdef PRINT_DEBUG
    printf("Frame sent: %.8X\n", __builtin_bswap32(send_frame.data32));
    printf("Offset: 0x%.2X, Content: %.8X\n", offset, __builtin_bswap32(receive_frame.data32));
#endif
    return receive_frame;
}

void alertCallback(uint gpio, uint32_t events) {
    uint64_t t1 = to_us_since_boot(get_absolute_time());
    SPI_frame send_frame, receive_frame;
    send_frame.byte_arr[0] = (X_CH_RESULT | READ_REG);
    send_frame.byte_arr[1] = 0x00;
    send_frame.byte_arr[2] = 0x00;
    // send_frame.byte_arr[3] = start_conversion;
    send_frame.byte_arr[3] = 0x00;
    uint8_t CRC = generateCRC(__builtin_bswap32(send_frame.data32));
    send_frame.byte_arr[3] |= CRC;

    gpio_put(SPI_CS, 0);
    for(int i = 0; i < 4; i++) {
        spi_read_blocking(spi0, send_frame.byte_arr[i], receive_frame.byte_arr + i, 1);
    }
    gpio_put(SPI_CS, 1);

    printf("%ld: %.8X\n", (long)t1, __builtin_bswap32(receive_frame.data32));

    return;
}