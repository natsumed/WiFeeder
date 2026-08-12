cmake_minimum_required(VERSION 3.16)
project(wifeeder_v2_mcu C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler flags
set(CPU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS "${CPU_FLAGS} -Wall -Wextra -Os -ffunction-sections -fdata-sections")
set(CMAKE_ASM_FLAGS "${CPU_FLAGS}")

# Define for STM32L4 HAL (uses STM32L486xx for compatibility - L432 is subset)
add_compile_definitions(
    STM32L486xx
    USE_HAL_DRIVER
    HSE_VALUE=8000000
)

# Include paths
set(INC_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src
    ${CMAKE_CURRENT_SOURCE_DIR}/Config
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32L4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/Drivers/STM32L4xx_HAL_Driver
)

include_directories(${INC_DIRS})

# HAL source files (minimal set)
set(HAL_SRCS
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_cortex.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_rcc.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_rcc_ex.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_flash.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_flash_ex.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_flash_ramfunc.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_gpio.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_dma.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_pwr.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_pwr_ex.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_spi.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_spi_ex.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_uart.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_uart_ex.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_tim.c
    Drivers/STM32L4xx_HAL_Driver/stm32l4xx_hal_tim_ex.c
)

# Application sources
set(APP_SRCS
    Core/Src/main.c
    Core/Src/syscalls.c
    Core/Src/board.c
    Core/Src/crc8.c
    Core/Src/protocol.c
    Core/Src/nrf24l01.c
    Core/Src/rfid.c
    Core/Src/pca9685.c
    Core/Src/motor.c
    Core/Src/encoder.c
    Core/Src/hx711.c
    Core/Src/flash_int.c
    Core/Src/wifeeder.c
)

# Startup
set(STARTUP_SRC startup/startup_stm32l486xx.s)

# Executable
add_executable(wifeeder_v2_mcu.elf ${APP_SRCS} ${HAL_SRCS} ${STARTUP_SRC})

# Linker script
target_link_options(wifeeder_v2_mcu.elf PRIVATE
    -T${CMAKE_CURRENT_SOURCE_DIR}/linker/STM32L432KC_FLASH.ld
    -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/wifeeder_v2_mcu.map
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -nostartfiles
)

# Post-build: generate .bin
add_custom_command(TARGET wifeeder_v2_mcu.elf POST_BUILD
    COMMAND arm-none-eabi-objcopy -O binary
        ${CMAKE_CURRENT_BINARY_DIR}/wifeeder_v2_mcu.elf
        ${CMAKE_CURRENT_BINARY_DIR}/wifeeder_v2_mcu.bin
    COMMAND arm-none-eabi-size
        ${CMAKE_CURRENT_BINARY_DIR}/wifeeder_v2_mcu.elf
    COMMENT "Generating .bin and size report"
)
