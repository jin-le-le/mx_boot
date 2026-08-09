/**
 * @file board_config.h
 * @brief STM32F103C8T6 board configuration
 *
 * Flash layout (64KB total, 1KB pages):
 *   0x08000000 - 0x08005FFF  Bootloader (24KB, 24 pages)
 *   0x08006000 - 0x080063FF  Header (1KB page, 256B used)
 *   0x08006400 - 0x0800FFFF  App (40KB, 40 pages)
 *
 * Note: the bootloader was previously sized at 20KB, but the actual build was
 *     21.4KB (including micro-ecc/HAL/elog). It overflowed past 0x08005400
 *     and was erased by its own erase command, causing a HardFault.
 *     After changing to 24KB, more than 2KB of headroom is reserved.
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*=======================================================================
 * Flash Layout (STM32F103C8T6 - 64KB total)
 *=======================================================================*/
#define BL_FLASH_BASE        0x08000000U
#define BL_FLASH_TOTAL      (64 * 1024)      /* 64KB total flash */
#define BL_BOOTLOADER_SIZE   (24 * 1024)      /* Bootloader: 24KB */
#define BL_APP_BASE          0x08006400U      /* Application start (after header page) */
#define BL_MAGIC_HEADER_ADDR 0x08006000U      /* Header at page 24 */

#define BL_BOOTLOADER_END    (BL_FLASH_BASE + BL_BOOTLOADER_SIZE)
#define BL_HEADER_SIZE       256

/*=======================================================================
 * UART Configuration
 *=======================================================================*/
#define BL_UART_INSTANCE     3               /* USART3 (protocol port PB10/PB11) */
#define BL_UART_BAUDRATE     115200

/*=======================================================================
 * Timer Configuration
 *=======================================================================*/
#define BL_TIMER_INSTANCE    2               /* TIM2 */

/*=======================================================================
 * Boot Trap - KEY (this board has no KEY, disabled)
 *=======================================================================*/
#define BL_ENABLE_KEY_TRAP   0

/*=======================================================================
 * GPIO Port Numbers
 *=======================================================================*/
#define GPIO_PORT_A   0
#define GPIO_PORT_B   1
#define GPIO_PORT_C   2
#define GPIO_PORT_D   3

/*=======================================================================
 * Chip-specific Flash Protection (STM32 RDP)
 *=======================================================================*/
#define BL_FLASH_PROTECTION_ENABLED  0       /* F103: not enabled yet */
#define BL_RDP_LEVEL             0

/*=======================================================================
 * Feature configuration (chip-independent)
 *=======================================================================*/
#include "bl_features.h"

#endif /* BOARD_CONFIG_H */
