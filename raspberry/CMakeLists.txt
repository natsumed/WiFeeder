cmake_minimum_required(VERSION 3.16)
project(wifeeder_host LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(RASPBERRY_PI "Build for Raspberry Pi with SPI/GPIO NRF driver" OFF)
option(WIFEEDER_USE_JSONCPP "Prefer jsoncpp over embedded min_json" ON)
option(WIFEEDER_USE_MOSQUITTO "Enable libmosquitto MQTT client" ON)
option(WIFEEDER_BUILD_TESTS "Build diet unit tests" ON)

if(RASPBERRY_PI)
    add_compile_definitions(RASPBERRY_PI)
    message(STATUS "Building for Raspberry Pi target")
else()
    message(STATUS "Building native/stub host (no SPI required)")
endif()

find_package(Threads REQUIRED)

set(WIFEEDER_SOURCES
    src/error.cpp
    src/protocol.cpp
    src/nrf24l01.cpp
    src/nrf_link.cpp
    src/diet_engine.cpp
    src/database.cpp
    src/mqtt_client.cpp
    src/main.cpp
)

if(WIFEEDER_USE_JSONCPP)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(JSONCPP QUIET jsoncpp)
    endif()
    if(JSONCPP_FOUND)
        add_compile_definitions(HAVE_JSONCPP)
        message(STATUS "Using jsoncpp")
    else()
        find_path(JSONCPP_INCLUDE_DIR json/json.h)
        find_library(JSONCPP_LIBRARY jsoncpp)
        if(JSONCPP_INCLUDE_DIR AND JSONCPP_LIBRARY)
            add_compile_definitions(HAVE_JSONCPP)
            set(JSONCPP_FOUND TRUE)
            set(JSONCPP_INCLUDE_DIRS ${JSONCPP_INCLUDE_DIR})
            set(JSONCPP_LIBRARIES ${JSONCPP_LIBRARY})
            message(STATUS "Using jsoncpp (find_library)")
        else()
            message(STATUS "jsoncpp not found; using embedded min_json")
            list(APPEND WIFEEDER_SOURCES src/min_json.cpp)
        endif()
    endif()
else()
    list(APPEND WIFEEDER_SOURCES src/min_json.cpp)
endif()

if(WIFEEDER_USE_MOSQUITTO)
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(MOSQUITTO QUIET libmosquittopp)
    endif()
    if(MOSQUITTO_FOUND)
        add_compile_definitions(HAVE_MOSQUITTO)
        message(STATUS "Using libmosquittopp")
    else()
        message(STATUS "libmosquittopp not found; MQTT stub enabled")
    endif()
endif()

add_executable(wifeeder_host ${WIFEEDER_SOURCES})
target_include_directories(wifeeder_host PRIVATE include)

target_link_libraries(wifeeder_host PRIVATE Threads::Threads)

if(JSONCPP_FOUND)
    if(TARGET PkgConfig::JSONCPP)
        target_link_libraries(wifeeder_host PRIVATE PkgConfig::JSONCPP)
    else()
        target_include_directories(wifeeder_host PRIVATE ${JSONCPP_INCLUDE_DIRS})
        target_link_libraries(wifeeder_host PRIVATE ${JSONCPP_LIBRARIES})
    endif()
endif()

if(MOSQUITTO_FOUND)
    if(TARGET PkgConfig::MOSQUITTO)
        target_link_libraries(wifeeder_host PRIVATE PkgConfig::MOSQUITTO)
    else()
        target_include_directories(wifeeder_host PRIVATE ${MOSQUITTO_INCLUDE_DIRS})
        target_link_libraries(wifeeder_host PRIVATE ${MOSQUITTO_LIBRARIES})
    endif()
endif()

if(WIFEEDER_BUILD_TESTS)
    enable_testing()
    find_package(GTest QUIET)
    if(GTest_FOUND)
        add_executable(test_diet
            tests/test_diet.cpp
            src/diet_engine.cpp
            src/error.cpp
        )
        target_include_directories(test_diet PRIVATE include)
        if(JSONCPP_FOUND)
            if(TARGET PkgConfig::JSONCPP)
                target_link_libraries(test_diet PRIVATE PkgConfig::JSONCPP)
            else()
                target_include_directories(test_diet PRIVATE ${JSONCPP_INCLUDE_DIRS})
                target_link_libraries(test_diet PRIVATE ${JSONCPP_LIBRARIES})
            endif()
            target_compile_definitions(test_diet PRIVATE HAVE_JSONCPP)
        else()
            target_sources(test_diet PRIVATE src/min_json.cpp)
        endif()
        target_link_libraries(test_diet PRIVATE GTest::gtest GTest::gtest_main Threads::Threads)
        add_test(NAME diet COMMAND test_diet)
    else()
        add_executable(test_diet
            tests/test_diet.cpp
            src/diet_engine.cpp
            src/error.cpp
        )
        target_include_directories(test_diet PRIVATE include)
        if(JSONCPP_FOUND)
            if(TARGET PkgConfig::JSONCPP)
                target_link_libraries(test_diet PRIVATE PkgConfig::JSONCPP)
            else()
                target_include_directories(test_diet PRIVATE ${JSONCPP_INCLUDE_DIRS})
                target_link_libraries(test_diet PRIVATE ${JSONCPP_LIBRARIES})
            endif()
            target_compile_definitions(test_diet PRIVATE HAVE_JSONCPP)
        else()
            target_sources(test_diet PRIVATE src/min_json.cpp)
        endif()
        target_link_libraries(test_diet PRIVATE Threads::Threads)
        add_test(NAME diet COMMAND test_diet)
    endif()
endif()

install(TARGETS wifeeder_host RUNTIME DESTINATION bin)
install(FILES config/config.json config/wifeeder-data.json DESTINATION share/wifeeder)
