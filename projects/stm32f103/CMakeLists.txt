cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_PROJECT_NAME MCU_BOOT)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
project(${CMAKE_PROJECT_NAME})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
endif()
message("Build type: " ${CMAKE_BUILD_TYPE})

enable_language(C ASM)
add_executable(${CMAKE_PROJECT_NAME})

# Boot core source path
set(BOOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../boot)

add_subdirectory(cmake/stm32cubemx)

set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/LinkerScript/STM32F103C8_Bootloader.ld)
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -T${LINKER_SCRIPT})

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    uECC_SUPPORTS_secp160r1=0
    uECC_SUPPORTS_secp192r1=0
    uECC_SUPPORTS_secp224r1=0
    uECC_SUPPORTS_secp256r1=1
    uECC_SUPPORTS_secp256k1=0
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    App/bitops.h App/utils.h App/board_config.h App/bl_features.h
    ${BOOT_DIR}/core/bl_main.c
    ${BOOT_DIR}/core/image.c
    ${BOOT_DIR}/core/protocol.c
    ${BOOT_DIR}/lib/crc/crc16.c
    ${BOOT_DIR}/lib/crc/crc32.c
    ${BOOT_DIR}/lib/sha256/sha256.c
    ${BOOT_DIR}/lib/ringbuffer/ringbuffer.c
    ${BOOT_DIR}/lib/elog/elog.c
    ${BOOT_DIR}/lib/elog/elog_utils.c
    ${BOOT_DIR}/lib/elog/elog_port.c
    ${BOOT_DIR}/lib/ecdsa/uECC.c
    port/port_flash.c
    port/port_gpio.c
    port/port_uart.c
    port/port_console_uart.c
    port/port_timer.c
    port/port_system.c
    port/stm32f1_port.c
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Inc
    Drivers/STM32F1xx_HAL_Driver/Inc
    App
    ${BOOT_DIR}
    ${BOOT_DIR}/core
    ${BOOT_DIR}/lib/crc
    ${BOOT_DIR}/lib/sha256
    ${BOOT_DIR}/lib/ringbuffer
    ${BOOT_DIR}/lib/elog
    ${BOOT_DIR}/lib/ecdsa
    port
)

list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
target_link_libraries(${CMAKE_PROJECT_NAME} stm32cubemx)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_link_options(-flto)
    add_link_options(-Wl,--gc-sections)
endif()
