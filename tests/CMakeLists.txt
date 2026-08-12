# Optional master CMake — each numbered test also builds standalone.
cmake_minimum_required(VERSION 3.16)
project(wifeeder_v2_tests NONE)

# STM32 tests require cross toolchain when building from here:
#   cmake -B build -DCMAKE_TOOLCHAIN_FILE=/home/luceor/Desktop/wifeeder/cmake/toolchain-arm-none-eabi.cmake
#   cmake --build build

option(WIFTEST_BUILD_STM32 "Build STM32 hardware tests (01-07)" OFF)
option(WIFTEST_BUILD_PI "Build Pi NRF tests (08)" OFF)

if(WIFTEST_BUILD_STM32)
    enable_language(C ASM)
    add_subdirectory(01-led)
    add_subdirectory(02-uart)
    add_subdirectory(03-nrf)
    add_subdirectory(04-motor)
    add_subdirectory(05-encoder)
    add_subdirectory(06-rfid)
    add_subdirectory(07-hx711)
endif()

if(WIFTEST_BUILD_PI)
    enable_language(CXX)
    add_subdirectory(08-nrf-pi)
endif()
