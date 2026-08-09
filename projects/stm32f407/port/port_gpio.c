/**
 * @file port_gpio.c
 * @brief GPIO interface implementation using HAL
 */

#include "../hal_if_defines.h"
#include "port.h"
#include "main.h"

static void enable_gpio_clock(uint8_t port)
{
    switch (port) {
        case 0: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
        case 1: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
        case 2: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
        case 3: __HAL_RCC_GPIOD_CLK_ENABLE(); break;
        case 4: __HAL_RCC_GPIOE_CLK_ENABLE(); break;
    }
}

static int port_gpio_init(uint8_t port, uint16_t pin, uint8_t mode, uint8_t pupd)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    enable_gpio_clock(port);

    GPIO_InitStructure.Pin = (uint32_t)(1UL << pin);
    GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;

    switch (mode) {
        case BL_GPIO_MODE_IN:
            GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
            break;
        case BL_GPIO_MODE_OUT:
            GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
            break;
        case BL_GPIO_MODE_AF:
            GPIO_InitStructure.Mode = GPIO_MODE_AF_PP;
            break;
        case BL_GPIO_MODE_ANALOG:
            GPIO_InitStructure.Mode = GPIO_MODE_ANALOG;
            break;
        default:
            return -1;
    }

    switch (pupd) {
        case GPIO_PUPD_NONE:
            GPIO_InitStructure.Pull = GPIO_NOPULL;
            break;
        case GPIO_PUPD_PULLUP:
            GPIO_InitStructure.Pull = GPIO_PULLUP;
            break;
        case GPIO_PUPD_PULLDOWN:
            GPIO_InitStructure.Pull = GPIO_PULLDOWN;
            break;
        default:
            return -1;
    }

    HAL_GPIO_Init(port_gpio_port(port), &GPIO_InitStructure);
    return 0;
}

static void port_gpio_set(uint8_t port, uint16_t pin, bool level)
{
    HAL_GPIO_WritePin(port_gpio_port(port), (uint32_t)(1UL << pin),
                      level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static bool port_gpio_read(uint8_t port, uint16_t pin)
{
    return HAL_GPIO_ReadPin(port_gpio_port(port), (uint32_t)(1UL << pin)) == GPIO_PIN_SET;
}

static void port_gpio_deinit(uint8_t port, uint16_t pin)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};

    GPIO_InitStructure.Pin = (uint32_t)(1UL << pin);
    GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
    GPIO_InitStructure.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(port_gpio_port(port), &GPIO_InitStructure);
}

const gpio_if_t gpio_stm32f4 = {
    .init    = port_gpio_init,
    .set     = port_gpio_set,
    .read    = port_gpio_read,
    .deinit  = port_gpio_deinit,
};
