#include <stdio.h>
#include "hardware/gpio.h"
#include "hardware/irq.h"
#include "hardware/spi.h"
#include "pico/stdlib.h"

#include "TMAG5170.hpp"
#define TMAG_ALERT 17

void alertCallback(uint, uint32_t);

TMAG5170 myTMAG;

int main(void) {
    stdio_init_all();
    sleep_ms(5000);
    printf("Hello world!\n");

    myTMAG.attachSPI(spi0, 18, 19, 20, 21, 2e6);
    myTMAG.init();
    myTMAG.setConversionAverage(CONV_AVG_32x);
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

void alertCallback(uint gpio, uint32_t events) {
    uint64_t t1 = to_us_since_boot(get_absolute_time());
    float Bx = myTMAG.readX();
    float By = myTMAG.readY();
    float Bz = myTMAG.readZ();
    printf("%lu: %.1f, %.1f, %.1f\n", (unsigned long)t1, Bx, By, Bz);

    return;
}