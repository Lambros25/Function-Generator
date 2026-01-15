cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(led_pwm C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(led_pwm
    main.cpp
)

target_link_libraries(led_pwm
    pico_stdlib
    hardware_pwm
)

pico_add_extra_outputs(led_pwm)

