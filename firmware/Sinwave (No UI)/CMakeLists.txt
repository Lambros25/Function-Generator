cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(learn C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(learn
    main.cpp
)

# Added pico_multicore to this list
target_link_libraries(learn
    pico_stdlib
    hardware_pwm
    hardware_spi
    pico_multicore 
)

pico_add_extra_outputs(learn)

# Fixed: Added '1' to enable USB and '0' to disable UART
pico_enable_stdio_usb(learn 1)
pico_enable_stdio_uart(learn 0)