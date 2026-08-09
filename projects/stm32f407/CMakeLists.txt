cmake_minimum_required(VERSION 3.22)

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# Set the project name
set(CMAKE_PROJECT_NAME MCU_BOOT)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})

# Optimization flags for size
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
endif()
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# ============================================================
#  Boot core source path (refers to the top-level boot/ directory)
# ============================================================
set(BOOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../boot)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Linker script
set(LINKER_SCRIPT ${CMAKE_SOURCE_DIR}/LinkerScript/STM32F407ZG_Bootloader.ld)

# Specify linker script for the executable
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE
    -T${LINKER_SCRIPT}
)

# Add compile definitions
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # micro-ecc: compile only P-256 curve, disable others to save Flash
    uECC_SUPPORTS_secp160r1=0
    uECC_SUPPORTS_secp192r1=0
    uECC_SUPPORTS_secp224r1=0
    uECC_SUPPORTS_secp256r1=1
    uECC_SUPPORTS_secp256k1=0
)

# ============================================================
#  Source files
# ============================================================
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # ---- App config (chip-specific) ----
    App/bitops.h
    App/utils.h
    App/board_config.h
    App/bl_features.h

    # ---- Boot core (from top-level boot/, chip-independent) ----
    ${BOOT_DIR}/core/bl_main.c
    ${BOOT_DIR}/core/image.c
    ${BOOT_DIR}/core/protocol.c

    # ---- Boot Lib (from top-level boot/, chip-independent) ----
    ${BOOT_DIR}/lib/crc/crc16.c
    ${BOOT_DIR}/lib/crc/crc32.c
    ${BOOT_DIR}/lib/sha256/sha256.c
    ${BOOT_DIR}/lib/ringbuffer/ringbuffer.c
    ${BOOT_DIR}/lib/elog/elog.c
    ${BOOT_DIR}/lib/elog/elog_utils.c
    ${BOOT_DIR}/lib/elog/elog_port.c
    ${BOOT_DIR}/lib/ecdsa/uECC.c

    # ---- Port layer (chip-specific) ----
    port/port_gpio.c
    port/port_flash.c
    port/port_uart.c
    port/port_console_uart.c
    port/port_timer.c
    port/port_system.c
    port/stm32f4_port.c
)

# ============================================================
#  Include paths
# ============================================================
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # CubeMX + HAL
    Core/Inc
    Drivers/STM32F4xx_HAL_Driver/Inc
    # Chip-specific config
    App
    # Boot core (from top-level boot/)
    ${BOOT_DIR}
    ${BOOT_DIR}/core
    ${BOOT_DIR}/lib/crc
    ${BOOT_DIR}/lib/sha256
    ${BOOT_DIR}/lib/ringbuffer
    ${BOOT_DIR}/lib/elog
    ${BOOT_DIR}/lib/ecdsa
    # Chip-specific port
    port
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx
)

# Link-time optimization and garbage collection for size reduction
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_link_options(-flto)
    add_link_options(-Wl,--gc-sections)
endif()
