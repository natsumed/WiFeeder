cmake_minimum_required(VERSION 3.16)
project(test_motor C ASM)

include(../cmake/stm32_test.cmake)

get_filename_component(WIFTEST_STM32_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../stm32" ABSOLUTE)

wifeeder_stm32_test(test_motor test_motor.elf
    SOURCES
        main.c
        ../common/test_utils.c
        ${WIFTEST_STM32_ROOT}/Core/Src/board.c
        ${WIFTEST_STM32_ROOT}/Core/Src/pca9685.c
        ${WIFTEST_STM32_ROOT}/Core/Src/motor.c
)

target_include_directories(test_motor PRIVATE ${WIFTEST_STM32_ROOT}/Core/Inc)
