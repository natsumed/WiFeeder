cmake_minimum_required(VERSION 3.16)
project(wifeeder_nrf_pi CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test_nrf_tx test_nrf_tx.cpp)
add_executable(test_nrf_rx test_nrf_rx.cpp)
