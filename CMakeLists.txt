cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pill_dispenser_pico C CXX ASM)
set(CMAKE_C_STANDARD 11)

pico_sdk_init()

add_executable(pill_dispenser
    src/main.c
)

target_link_libraries(pill_dispenser
    pico_stdlib
    hardware_i2c
)

pico_add_extra_outputs(pill_dispenser)
