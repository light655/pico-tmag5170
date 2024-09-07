#include <stdio.h>
#include <string.h>
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"
#include "pico/stdio.h"

#include "TMAG5170.hpp"
#define TMAG_ALERT 17

typedef union {
    uint32_t unsigned32;
    uint8_t byte_arr[4];
} container32_t;

void alertCallback(uint, uint32_t);

int32_t num_measure = 0;
TMAG5170 myTMAG;

int main(void) {
    stdio_init_all();

    myTMAG.attachSPI(spi0, 18, 19, 20, 21, 2e6);    // attach SPI ad 2MHz
    myTMAG.init();
    myTMAG.setConversionAverage(CONV_AVG_1x);
    myTMAG.enableMagneticChannel(true, true, true);
    myTMAG.setMagneticRange(X_RANGE_300mT, Y_RANGE_300mT, Z_RANGE_300mT);

    while(true) {
        while(getchar_timeout_us(1000) != 'S') {
            tight_loop_contents();                  // spin the program until receiving the start signal
        }
        sleep_ms(10);
        container32_t tmp;
        stdio_get_until((char*)tmp.byte_arr, 4, 0); // read the number of measurements to perform to perform
        num_measure = (int32_t)tmp.unsigned32;

        myTMAG.setOperatingMode(OPERATING_MODE_ActiveMeasureMode);
        myTMAG.enableAlertOutput(true);             // enable ALERT output for conversion ready interrupt

        gpio_set_irq_enabled_with_callback(TMAG_ALERT, GPIO_IRQ_EDGE_FALL, true, &alertCallback);
        while(num_measure > 0) {
            tight_loop_contents();                  // spin the processor and wait for interrupts
        }
        gpio_set_irq_enabled_with_callback(TMAG_ALERT, GPIO_IRQ_EDGE_FALL, false, &alertCallback);
        myTMAG.setOperatingMode(OPERATING_MODE_ConfigurationMode);
        myTMAG.enableAlertOutput(false);
    }

    return 0;
}

union measure_frame {
    struct {
        uint32_t t1;
        int16_t Bx;
        int16_t By;
        int16_t Bz;
    } S;
    char arr[10];
};

void alertCallback(uint gpio, uint32_t events) {
    measure_frame frame;
    frame.S.t1 = to_us_since_boot(get_absolute_time());
    frame.S.Bx = myTMAG.readXRaw();
    frame.S.By = myTMAG.readYRaw();
    frame.S.Bz = myTMAG.readZRaw();
    stdio_put_string(frame.arr, 10, false, false);
    num_measure--;
    
    return;
}