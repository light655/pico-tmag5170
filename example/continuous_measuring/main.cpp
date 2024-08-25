#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

#include "TMAG5170.hpp"
#define TMAG_ALERT 17

void initTMAG(void);
void alertCallback(uint, uint32_t);

TMAG5170 myTMAG;

int main(void) {
    stdio_init_all();
    sleep_ms(5000);
    printf("Hello world!\n");

    myTMAG.attachSPI(spi0, 18, 19, 20, 21, 1e6);
    myTMAG.init();
    myTMAG.setConversionAverage(CONV_AVG_1x);
    myTMAG.setOperatingMode(OPERATING_MODE_ActiveMeasureMode);
    myTMAG.enableMagneticChannel(true, true, true);
    myTMAG.setMagneticRange(X_RANGE_300mT, Y_RANGE_300mT, Z_RANGE_300mT);
    myTMAG.enableAlertOutput(true);

    gpio_set_irq_enabled_with_callback(TMAG_ALERT, GPIO_IRQ_EDGE_FALL, true, &alertCallback);

    while(true) {
        tight_loop_contents();
    }

    return 0;
}

void initTMAG(void) {
    uint16_t data16;

    for(uint8_t offset = 0x15; offset <= 0x15; offset--) {
        switch(offset) {
        case DEVICE_CONFIG:
            data16 = CONV_AVG_32x | OPERATING_MODE_ActiveMeasureMode;
            myTMAG.writeRegister(offset, data16);
            data16 = myTMAG.readRegister(offset);
            printf("Offset: %.2X; Content: %.4X\n", offset, data16);
            break;
        case SENSOR_CONFIG:
            data16 = MAG_CH_EN_XYZenabled | Z_RANGE_300mT | Y_RANGE_300mT | X_RANGE_300mT;
            myTMAG.writeRegister(offset, data16);
            data16 = myTMAG.readRegister(offset);
            printf("Offset: %.2X; Content: %.4X\n", offset, data16);
            break;
        case ALERT_CONFIG:
            data16 = RSLT_ALRT_Asserted;
            myTMAG.writeRegister(offset, data16);
            data16 = myTMAG.readRegister(offset);
            printf("Offset: %.2X; Content: %.4X\n", offset, data16);
            break;
        default:
            break;
        }
    }

    return;
}

void alertCallback(uint gpio, uint32_t events) {
    uint64_t t1 = to_us_since_boot(get_absolute_time());
    uint16_t X_result = myTMAG.readRegister(X_CH_RESULT);
    // printf("%lu: %.4X\n", (unsigned long)t1, X_result);

    return;
}