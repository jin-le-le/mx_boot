/**
 * @file port.h
 * @brief STM32F4 port common definitions
 */

#ifndef PORT_STM32F4_H
#define PORT_STM32F4_H

#include <stddef.h>
#include "stm32f4xx_hal.h"

/*=======================================================================
 * Chip Configuration (from board_config.h)
 *=======================================================================*/
#ifndef BL_FLASH_BASE
#define BL_FLASH_BASE      0x08000000
#endif

#ifndef BL_FLASH_TOTAL
#define BL_FLASH_TOTAL    (1024 * 1024)  /* STM32F407ZG: 1024KB */
#endif

#ifndef BL_BOOTLOADER_SIZE
#define BL_BOOTLOADER_SIZE (48 * 1024)
#endif

#ifndef BL_APP_BASE
#define BL_APP_BASE       0x08010000
#endif

#ifndef BL_MAGIC_HEADER_ADDR
#define BL_MAGIC_HEADER_ADDR 0x0800C000
#endif

#ifndef BL_UART_INSTANCE
#define BL_UART_INSTANCE  3
#endif

#ifndef BL_TIMER_INSTANCE
#define BL_TIMER_INSTANCE 6
#endif

/*=======================================================================
 * Flash Sector Definitions (STM32F4)
 *=======================================================================*/
#define STM32F4_SECTOR_SIZE_0     (16 * 1024)
#define STM32F4_SECTOR_SIZE_1     (16 * 1024)
#define STM32F4_SECTOR_SIZE_2     (16 * 1024)
#define STM32F4_SECTOR_SIZE_3     (16 * 1024)
#define STM32F4_SECTOR_SIZE_4     (64 * 1024)
#define STM32F4_SECTOR_SIZE_5     (128 * 1024)
#define STM32F4_SECTOR_SIZE_6     (128 * 1024)
#define STM32F4_SECTOR_SIZE_7     (128 * 1024)
#define STM32F4_SECTOR_SIZE_8     (128 * 1024)
#define STM32F4_SECTOR_SIZE_9     (128 * 1024)
#define STM32F4_SECTOR_SIZE_10    (128 * 1024)
#define STM32F4_SECTOR_SIZE_11    (128 * 1024)

/*=======================================================================
 * Helper Macros
 *=======================================================================*/
#define PORT_STM32F4_BIT(n)  ((uint32_t)(1UL << (n)))

/**
 * @brief Convert GPIO port number to GPIO_TypeDef*
 */
static inline GPIO_TypeDef *port_gpio_port(uint8_t port)
{
    switch (port) {
        case 0: return GPIOA;
        case 1: return GPIOB;
        case 2: return GPIOC;
        case 3: return GPIOD;
        case 4: return GPIOE;
        default: return NULL;
    }
}

#endif /* PORT_STM32F4_H */
