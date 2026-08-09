/**
 * @file board_config.h
 * @brief Board-level configuration for STM32F407 Bootloader
 *
 * ====== Chip + board-level configuration ======
 *
 * Modify this file when porting to a new chip/board. Business feature
 * configuration lives in bl_features.h and is chip-independent; this file
 * will automatically #include "bl_features.h" at the end.
 *
 * STM32F407ZGT6: 1024KB Flash, 192KB RAM
 * - Bootloader: 0x08000000 - 0x0800BFFF (48KB)
 * - App:        0x08010000 - 0x0800FFFF (896KB, last 64KB reserved for header)
 * - Header:     0x0800C000 - 0x0800FFFF (16KB)
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*=======================================================================
 * Flash Layout (STM32F407ZGT6 - 1024KB total)
 *=======================================================================*/
#define BL_FLASH_BASE        0x08000000U
#define BL_FLASH_TOTAL      (1024 * 1024)    /* 1024KB total flash */
#define BL_BOOTLOADER_SIZE   (48 * 1024)      /* Bootloader: 48KB */
#define BL_APP_BASE          0x08010000U      /* Application vector table */
#define BL_MAGIC_HEADER_ADDR 0x0800C000U       /* Magic header in sector 11 */

/* Flash protection boundaries */
#define BL_BOOTLOADER_END    (BL_FLASH_BASE + BL_BOOTLOADER_SIZE)  /* 0x0800C000 */
#define BL_HEADER_SIZE       256                                     /* Header area size */

/*=======================================================================
 * UART Configuration
 *=======================================================================*/
/* Bootloader communication UART (USART3, PC10/PC11) */
#define BL_UART_INSTANCE     3               /* USART3 */
#define BL_UART_BAUDRATE     115200          /* 115200 8-N-1 */

/*=======================================================================
 * Timer Configuration
 *=======================================================================*/
#define BL_TIMER_INSTANCE    6               /* TIM6 for delays and tick */

/*=======================================================================
 * Boot Trap - KEY (board-specific)
 *=======================================================================*/
/* KEY trap switch
 *   1 = enabled (press KEY to enter bootloader)
 *   0 = disabled (for products without a KEY button)*/
#define BL_ENABLE_KEY_TRAP   0

/* KEY pin (PA15 according to CubeMX configuration) */
#define BL_KEY_TRAP_PORT     GPIO_PORT_A
#define BL_KEY_TRAP_PIN      15              /* PA15 */

/*=======================================================================
 * Chip-specific Flash Protection (STM32 RDP)
 *=======================================================================*/
/* Master switch for Flash write protection */
#define BL_FLASH_PROTECTION_ENABLED  1

/* RDP Level (STM32 specific):
 *   0 = No protection
 *   1 = Level 1 (debug port can read RAM but not Flash)
 *   2 = Level 2 (permanently disables debug port, irreversible!) */
#define BL_RDP_LEVEL             1

/*=======================================================================
 * GPIO Port Numbers (HAL abstraction, used for GPIO pins in board_config.h)
 *=======================================================================*/
#define GPIO_PORT_A   0
#define GPIO_PORT_B   1
#define GPIO_PORT_C   2
#define GPIO_PORT_D   3
#define GPIO_PORT_E   4

/*=======================================================================
 * Business feature configuration (chip-independent)
 *=======================================================================
 * Business-related configuration (protocol, signature, CRC, logging,
 * trap pattern, etc.) lives in bl_features.h and does **not** need to
 * change when porting to a new chip.
 *
 * It is included automatically, so business code only needs
 * #include "board_config.h" to get all macros (kept for backward
 * compatibility, no changes required for existing code).
 *=======================================================================*/
#include "bl_features.h"

#endif /* BOARD_CONFIG_H */
