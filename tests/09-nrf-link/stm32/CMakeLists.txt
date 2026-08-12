cmake_minimum_required(VERSION 3.16)
project(test_nrf_link_stm C ASM)

include(../../cmake/stm32_test.cmake)

# RX peer (listens for PING, replies PONG)
wifeeder_stm32_test(test_nrf_link_rx test_nrf_link_rx.elf
    SOURCES main.c
)

# TX client (sends PING, waits for PONG)
wifeeder_stm32_test(test_nrf_link_tx test_nrf_link_tx.elf
    SOURCES main_tx.c
)
