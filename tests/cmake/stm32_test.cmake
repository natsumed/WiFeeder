# Shared CMake helpers for WiFeeder v2 STM32 hardware tests (NUCLEO-L432KC).
# Include from each tests/NN-name/CMakeLists.txt via: include(../cmake/stm32_test.cmake)

get_filename_component(WIFTEST_STM32_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../stm32" ABSOLUTE)

set(WIFTEST_CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

if(NOT DEFINED WIFTEST_FLAGS_INITIALIZED)
    set(CMAKE_C_STANDARD 99)
    set(CMAKE_C_STANDARD_REQUIRED ON)
    set(CMAKE_C_FLAGS "${WIFTEST_CPU_FLAGS} -Wall -Wextra -Os -ffunction-sections -fdata-sections")
    set(CMAKE_ASM_FLAGS "${WIFTEST_CPU_FLAGS}")
    set(WIFTEST_FLAGS_INITIALIZED 1 CACHE INTERNAL "")
endif()

# wifeeder_stm32_test(<target> <elf_name> [USE_HAL] [SKIP_DEFAULT_SYSCALLS] SOURCES ...)
function(wifeeder_stm32_test TARGET ELF_NAME)
    set(options USE_SYSCALLS USE_HAL SKIP_DEFAULT_SYSCALLS)
    set(oneValueArgs "")
    set(multiValueArgs SOURCES DEFINES)
    cmake_parse_arguments(WST "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT WST_SOURCES)
        message(FATAL_ERROR "wifeeder_stm32_test: SOURCES required")
    endif()

    set(_defs STM32L486xx)
    if(WST_USE_HAL)
        list(APPEND _defs USE_HAL_DRIVER HSE_VALUE=8000000)
    endif()
    if(WST_DEFINES)
        list(APPEND _defs ${WST_DEFINES})
    endif()
    add_compile_definitions(${_defs})

    set(_inc
        ${WIFTEST_STM32_ROOT}/Config
        ${WIFTEST_STM32_ROOT}/Drivers/CMSIS/Include
        ${WIFTEST_STM32_ROOT}/Drivers/CMSIS/Device/ST/STM32L4xx/Include
    )
    if(WST_USE_HAL)
        list(APPEND _inc ${WIFTEST_STM32_ROOT}/Drivers/STM32L4xx_HAL_Driver/Inc)
    endif()

    set(_src ${WST_SOURCES}
        ${WIFTEST_STM32_ROOT}/startup/startup_stm32l486xx.s
    )
    if(NOT WST_SKIP_DEFAULT_SYSCALLS)
        list(APPEND _src ${WIFTEST_STM32_ROOT}/Core/Src/syscalls.c)
    endif()

    add_executable(${TARGET} ${_src})
    get_filename_component(_base "${ELF_NAME}" NAME_WE)
    set_target_properties(${TARGET} PROPERTIES OUTPUT_NAME ${_base} SUFFIX ".elf")
    target_include_directories(${TARGET} PRIVATE ${_inc} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_LIST_DIR}/../common)

    # Toolchain already sets --specs=nano.specs; do not add another --specs=*.
    target_link_options(${TARGET} PRIVATE
        -T${WIFTEST_STM32_ROOT}/linker/STM32L432KC_FLASH.ld
        -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/${_base}.map
        -Wl,--gc-sections
        -Wl,--print-memory-usage
        -nostartfiles
    )

    add_custom_command(TARGET ${TARGET} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:${TARGET}>
            ${CMAKE_CURRENT_BINARY_DIR}/${_base}.bin
        COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${TARGET}>
        COMMENT "Generating ${_base}.bin and size report"
    )
endfunction()
