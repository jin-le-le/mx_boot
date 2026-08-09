#===============================================================================
# HC32F460PETB App Printf Project - Validate the GCC + CMake + DDL toolchain
#
# Goal: build an app that can printf "Hello HC32" to validate the build chain.
# Does not involve the bootloader.
#===============================================================================

cmake_minimum_required(VERSION 3.22)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_PROJECT_NAME hc32_app_printf)
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
project(${CMAKE_PROJECT_NAME})

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Os -g")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffunction-sections -fdata-sections")
endif()
message("Build type: " ${CMAKE_BUILD_TYPE})

enable_language(C ASM)
add_executable(${CMAKE_PROJECT_NAME})

# HC32 DDL SDK path (referenced in place, not copied)
set(SDK_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../examples/HC32F460_DDL_Rev3.3.0)

# Linker script
# Stage D: switch to the App version (FLASH starts at 0x0000E000 so the BOOT jump PC is valid)
set(LINKER_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/config/linker/HC32F460_App.ld)
target_link_options(${CMAKE_PROJECT_NAME} PRIVATE -T${LINKER_SCRIPT})

# All HC32 DDL driver source files (43 of them; let --gc-sections drop the unused ones)
file(GLOB DDL_SRC
    ${SDK_DIR}/drivers/hc32_ll_driver/src/*.c
)
list(REMOVE_ITEM DDL_SRC
    # USB requires the external usb_app_conf.h config file; this project does not use USB, so drop it
    ${SDK_DIR}/drivers/hc32_ll_driver/src/hc32_ll_usb.c
)

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # HC32 system initialization + startup file
    Core/Src/system_hc32f460petb.c
    Core/Src/startup_hc32f460petb.S

    # User application code
    # Switch test: pick one main file
    # - main.c             : Final App (USART1 printf + LED, runs from 0x0000E000)
    # - main_usart4_echo.c : USART4 echo test
    # - main_soft_uart.c   : Software-simulated UART
    # - main_gpio_test.c   : Pure GPIO toggle
    Core/Src/main.c
    Core/Src/syscalls.c

    # All DDL drivers
    ${DDL_SRC}
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    Core/Inc
    ${SDK_DIR}/drivers/hc32_ll_driver/inc
    ${SDK_DIR}/drivers/cmsis/Include
    ${SDK_DIR}/drivers/cmsis/Device/HDSC/hc32f4xx/Include
)

# Macros required by HC32 DDL
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    HC32F460
    _HC32F460_    # Some DDL files use this macro
    USE_DDL_DRIVER  # Critical: enables inclusion of all DDL modules in hc32_ll.h
)

# Output .bin / .hex / .dis
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
