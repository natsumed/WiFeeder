cmake_minimum_required(VERSION 3.16)
project(test_uart C ASM)

include(../cmake/stm32_test.cmake)

wifeeder_stm32_test(test_uart test_uart.elf
    SKIP_DEFAULT_SYSCALLS
    SOURCES
        main.c
        ../common/test_utils.c
)
