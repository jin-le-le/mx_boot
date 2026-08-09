#===============================================================================
# HC32F460 Bootloader project
#
# Reuses boot/core + boot/lib (100% chip-independent), implements 7 port hooks
#===============================================================================

cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_PROJECT_NAME hc32_boot)
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
set(BOOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../../boot)

# HC32 DDL path (already copied into this project's Drivers/)
set(DDL_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Drivers)

# Linker script (includes ASSERT to prevent bootloader self-overflow)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/LinkerScript/HC32F460_Bootloader.ld)
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -T${LINKER_SCRIPT})

# HC32 DDL required macros
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    HC32F460
    _HC32F460_
    USE_DDL_DRIVER    # CRITICAL: DDL module include master switch
)

# ECDSA uses only P-256 curve; disable others to save ~3KB Flash
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    uECC_SUPPORTS_secp160r1=0
    uECC_SUPPORTS_secp192r1=0
    uECC_SUPPORTS_secp224r1=0
    uECC_SUPPORTS_secp256r1=1
    uECC_SUPPORTS_secp256k1=0
)

# Pull in every HC32 DDL source; --gc-sections strips unused ones
file(GLOB DDL_SRC
    ${DDL_DIR}/hc32_ll_driver/src/*.c
)
list(REMOVE_ITEM DDL_SRC
    # USB module needs an external usb_app_conf.h; bootloader does not use it
    ${DDL_DIR}/hc32_ll_driver/src/hc32_ll_usb.c
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # HC32 system init + startup file
    Core/Src/system_hc32f460petb.c
    Core/Src/startup_hc32f460petb.S

    # Bootloader entry point
    Core/Src/main.c

    # newlib syscall retarget
    Core/Src/syscalls.c

    # Boot core code (100% chip-independent)
    ${BOOT_DIR}/core/bl_main.c
    ${BOOT_DIR}/core/image.c
    ${BOOT_DIR}/core/protocol.c

    # Boot internal libraries (100% chip-independent)
    ${BOOT_DIR}/lib/crc/crc16.c
    ${BOOT_DIR}/lib/crc/crc32.c
    ${BOOT_DIR}/lib/sha256/sha256.c
    ${BOOT_DIR}/lib/ringbuffer/ringbuffer.c
    ${BOOT_DIR}/lib/elog/elog.c
    ${BOOT_DIR}/lib/elog/elog_utils.c
    ${BOOT_DIR}/lib/elog/elog_port.c
    ${BOOT_DIR}/lib/ecdsa/uECC.c

    # HC32 port layer (one implementation per chip)
    port/port_flash.c
    port/port_gpio.c
    port/port_uart.c
    port/port_console_uart.c
    port/port_timer.c
    port/port_system.c
    port/hc32f4_port.c

    # HC32 DDL drivers
    ${DDL_SRC}
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Inc
    App
    ${DDL_DIR}/hc32_ll_driver/inc
    ${DDL_DIR}/cmsis/Include
    ${DDL_DIR}/cmsis/Device/HDSC/hc32f4xx/Include
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

# Emit bin / hex / dis artifacts
add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
            ${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
            ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJDUMP} -D $<TARGET_FILE:${CMAKE_PROJECT_NAME}>
            > ${CMAKE_PROJECT_NAME}.dis
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Generating bin/hex/dis artifacts"
)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    add_link_options(-flto)
    add_link_options(-Wl,--gc-sections)
endif()
