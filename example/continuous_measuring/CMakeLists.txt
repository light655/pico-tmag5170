cmake_minimum_required(VERSION 3.25)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

include(pico_sdk_import.cmake)

project(TMAG5170_continuous C CXX ASM)

pico_sdk_init()

add_executable(TMAG5170_continuous
    main.cpp
)

add_library(TMAG5170
    ../../src/TMAG5170.cpp
)

target_link_libraries(TMAG5170
    pico_stdlib
    hardware_irq
    hardware_spi
)

target_include_directories(TMAG5170_continuous
    PUBLIC ../../src/
)

target_link_libraries(TMAG5170_continuous
    pico_stdlib
    hardware_irq
    hardware_spi
    TMAG5170
)

pico_enable_stdio_usb(TMAG5170_continuous 0)
pico_enable_stdio_uart(TMAG5170_continuous 1)

pico_add_extra_outputs(TMAG5170_continuous)
