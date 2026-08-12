cmake_minimum_required(VERSION 3.16)
project(test_encoder C ASM)

include(../cmake/stm32_test.cmake)

wifeeder_stm32_test(test_encoder test_encoder.elf
    SOURCES
        main.c
        ../common/test_utils.c
)
