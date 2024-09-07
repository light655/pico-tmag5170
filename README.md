# pico-tmag5170
A TMAG5170 Hall-effect sensor library for Raspberry Pi Pico SDK.

## Intro
TMAG5170 is a high-precision 3D Hall-effect sensor IC designed by Texas Instruments. It measures the magnetic field in all 3 axes, with a sampling rate of up to 10 kSPS. Communication with TMAG5170 is based on SPI, with maximum frequency of 10 MHz and cyclic redundancy check. There are two versions, A1 and A2, of TMAG5170 with different measuring ranges.

## Usage
- Copy the two files in src (TMAG5170.cpp, TMAG5170.hpp) into your pico project.
- Add the TMAG5170 library to your CMakeLists.txt
```cmake
add_library(TMAG5170 TMAG5170.cpp)

target_link_libraries(TMAG5170
    pico_stdlib
    hardware_spi
)
```
- Link it to your project in your CMakeLists.txt
```cmake
target_link_libraries(your_project_name
    TMAG5170
    # the following two libraries will be needed too.
    pico_stdlib
    hardware_spi
)
```
- Remember to include the header file in your code.
```C
#include "TMAG5170.h"
```

## Notes

- This library uses version 2.0.0+ Pico-SDK
- The SPI functions in this library is written in a blocking fashion in order to implement CRC with every transaction with the TMAG5170.
- To operate the TMAG5170 at maximum speed, a SPI clock frequency of 2 MHz or more is required.

## Examples

### Continuous conversion

In this example, the device is configured to measure the magnetic field on all 3 axes. The output of the device will be the average of 32 conversion results, giving a sampling rate of 312.5 SPS. The nALERT output of the device is configured to assert a negative pulse when the conversion result is ready. This signal is used to generate an interrupt on the Pico. The Pico will read the conversion result and print it over UART.

Note that the data in the example is printed to UART. To print it over USB, swap the 0 and 1 in lines 36 and 37 in examples/continuous_measuring/CMakeLists.txt

### Measure and send to Python client

In this example, the device is configured to measure the magnetic field on all 3 axes at maximum speed. The measured data is sent as bytes through USB to a Python script. The device will perform a number of conversions(set in the Python program's input) at 10000 SPS.

Note that stdio has to be over USB for this example to work as intended. The pyserial module is required for the Python script.

## References
- [TMAG5170 datasheet](https://www.ti.com/lit/ds/symlink/tmag5170.pdf?ts=1724587283732&ref_url=https%253A%252F%252Fwww.ti.com%252Fproduct%252FTMAG5170)
- [My blog on more details about the TMAG5170](https://hackmd.io/8zu9jVyARfWYGYuD4Jl_aQ)