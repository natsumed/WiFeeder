cmake_minimum_required(VERSION 3.16)
project(test_rfid C ASM)

include(../cmake/stm32_test.cmake)

wifeeder_stm32_test(test_rfid test_rfid.elf
    SOURCES
        main.c
        ../common/test_utils.c
)
