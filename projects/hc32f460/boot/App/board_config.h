/**
 * @file board_config.h
 * @brief HC32F460PETB board-level configuration
 *
 * Flash layout (512KB total, 8KB sector):
 *   0x00000000 - 0x0000BFFF  Bootloader (48KB, 6 sectors)
 *   0x0000C000 - 0x0000DFFF  Header     (8KB, 1 sector, 256B actually used)
 *   0x0000E000 - 0x0007FFFF  App        (456KB, 57 sectors)
 *
 * ⚠️ Key differences between HC32 and STM32:
 *   - Flash base address is 0x00000000 (STM32 is 0x08000000)
 *   - The host tool reads addresses dynamically via device_info_t, no hardcoding required
 */

#ifndef BOARD_CONFIG_H
#define BOARD_CONFIG_H

/*=======================================================================
 * Flash Layout (HC32F460PETB - 512KB total)
 *=======================================================================*/
#define BL_FLASH_BASE        0x00000000U     /* ★HC32 base address, differs from STM32 */
#define BL_FLASH_TOTAL       (512 * 1024)    /* 512KB total flash */
#define BL_BOOTLOADER_SIZE   (48 * 1024)     /* Bootloader: 48KB (6 sectors × 8KB) */
#define BL_APP_BASE          0x0000E000U     /* Application start (sector 7) */
#define BL_MAGIC_HEADER_ADDR 0x0000C000U     /* Header at sector 6 */

#define BL_BOOTLOADER_END    (BL_FLASH_BASE + BL_BOOTLOADER_SIZE)
#define BL_HEADER_SIZE       256

/*=======================================================================
 * UART Configuration
 *
 * Dual-UART design (consistent with F407: printf and protocol port separated):
 *   - USART1 (PA10 TX)  → printf / elog debug output
 *   - USART4 (PE6/PB9)  → upgrade protocol port
 *=======================================================================*/
#define BL_CONSOLE_UART_INSTANCE   1        /* USART1 = printf port */
#define BL_UART_INSTANCE           4        /* USART4 = protocol port */
#define BL_UART_BAUDRATE           115200

/*=======================================================================
 * Timer Configuration
 *
 * Uses SysTick (built into Cortex-M4), no TIM peripheral required.
 * Same scheme as F103: port_timer.c uses HAL_GetTick/HAL_Delay equivalents.
 *=======================================================================*/
#define BL_TIMER_INSTANCE    0              /* 0 = use SysTick, no TIM peripheral */

/*=======================================================================
 * Boot Trap - KEY (HC32F460 board KEY is on PD3; KEY trap not enabled for now)
 *=======================================================================*/
#define BL_ENABLE_KEY_TRAP   0

/*=======================================================================
 * GPIO Port Numbers
 *=======================================================================*/
#define GPIO_PORT_A   0
#define GPIO_PORT_B   1
#define GPIO_PORT_C   2
#define GPIO_PORT_D   3
#define GPIO_PORT_E   4

/*=======================================================================
 * Chip-specific Flash protection (not enabled on HC32F460)
 *=======================================================================*/
#define BL_FLASH_PROTECTION_ENABLED  0
#define BL_RDP_LEVEL             0

/*=======================================================================
 * Feature configuration (chip-independent)
 *=======================================================================*/
#include "bl_features.h"

#endif /* BOARD_CONFIG_H */
