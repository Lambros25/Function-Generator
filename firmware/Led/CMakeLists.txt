cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(learn C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(learn
    main.cpp
)

target_link_libraries(learn
    pico_stdlib
    hardware_pwm
    hardware_spi
)

pico_add_extra_outputs(learn)

