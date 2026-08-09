/**
 * @file port.h
 * @brief STM32F103 port common definitions
 */

#ifndef PORT_STM32F1_H
#define PORT_STM32F1_H

#include <stddef.h>
#include "stm32f1xx_hal.h"

/* STM32F103 Flash: 1KB pages (medium density) */
#define STM32F1_PAGE_SIZE  (1 * 1024)

/* Helper: GPIO port number → GPIO_TypeDef* */
static inline GPIO_TypeDef *port_gpio_port(uint8_t port)
{
    switch (port) {
        case 0: return GPIOA;
        case 1: return GPIOB;
        case 2: return GPIOC;
        case 3: return GPIOD;
        default: return NULL;
    }
}

#endif /* PORT_STM32F1_H */
