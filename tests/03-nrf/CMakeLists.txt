cmake_minimum_required(VERSION 3.16)
project(test_nrf C ASM)

include(../cmake/stm32_test.cmake)

wifeeder_stm32_test(test_nrf test_nrf.elf
    SOURCES main.c
)
