cmake_minimum_required(VERSION 3.16)
project(test_nrf_rf_stm C ASM)

include(../../cmake/stm32_test.cmake)

wifeeder_stm32_test(test_nrf_rf_carrier test_nrf_rf_carrier.elf
    SOURCES main_carrier.c
)

wifeeder_stm32_test(test_nrf_rf_rpd test_nrf_rf_rpd.elf
    SOURCES main_rpd.c
)
