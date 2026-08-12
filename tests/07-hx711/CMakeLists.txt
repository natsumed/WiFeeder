cmake_minimum_required(VERSION 3.16)
project(test_hx711 C ASM)

include(../cmake/stm32_test.cmake)

wifeeder_stm32_test(test_hx711 test_hx711.elf
    SOURCES
        main.c
        ../common/test_utils.c
)
