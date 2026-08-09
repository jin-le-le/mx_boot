/**
 * @file port_gpio.c
 * @brief STM32F103 GPIO interface
 *
 * F103 HAL_GPIO_Init() handles the CRL/CRH registers internally; the API is
 * compatible with F407. The main difference: the GPIO mode enum values on F103
 * differ from F407 and require mapping.
 */

#include "../hal_if_defines.h"
#include "port.h"

static void enable_gpio_clock(GPIO_TypeDef *port)
{
    if (port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
    else if (port == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
    else if (port == GPIOD) __HAL_RCC_GPIOD_CLK_ENABLE();
}

static int port_gpio_init(uint8_t port, uint16_t pin, uint8_t mode, uint8_t pupd)
{
    GPIO_TypeDef *gpio = port_gpio_port(port);
    if (!gpio) return -1;

    enable_gpio_clock(gpio);

    GPIO_InitTypeDef gp = {0};
    gp.Pin = (uint32_t)(1UL << pin);
    gp.Speed = GPIO_SPEED_FREQ_HIGH;

    switch (mode) {
        case BL_GPIO_MODE_IN:
            gp.Mode = GPIO_MODE_INPUT;
            break;
        case BL_GPIO_MODE_OUT:
            gp.Mode = GPIO_MODE_OUTPUT_PP;
            break;
        case BL_GPIO_MODE_AF:
            gp.Mode = GPIO_MODE_AF_PP;
            break;
        case BL_GPIO_MODE_ANALOG:
            gp.Mode = GPIO_MODE_ANALOG;
            break;
        default:
            return -1;
    }

    switch (pupd) {
        case GPIO_PUPD_NONE:     gp.Pull = GPIO_NOPULL; break;
        case GPIO_PUPD_PULLUP:   gp.Pull = GPIO_PULLUP; break;
        case GPIO_PUPD_PULLDOWN: gp.Pull = GPIO_PULLDOWN; break;
        default: gp.Pull = GPIO_NOPULL; break;
    }

    HAL_GPIO_Init(gpio, &gp);
    return 0;
}

static void port_gpio_set(uint8_t port, uint16_t pin, bool level)
{
    GPIO_TypeDef *gpio = port_gpio_port(port);
    if (gpio) HAL_GPIO_WritePin(gpio, (uint16_t)(1UL << pin),
                                level ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

static bool port_gpio_read(uint8_t port, uint16_t pin)
{
    GPIO_TypeDef *gpio = port_gpio_port(port);
    if (!gpio) return false;
    return HAL_GPIO_ReadPin(gpio, (uint16_t)(1UL << pin)) == GPIO_PIN_SET;
}

static void port_gpio_deinit(uint8_t port, uint16_t pin)
{
    GPIO_TypeDef *gpio = port_gpio_port(port);
    if (gpio) HAL_GPIO_DeInit(gpio, (uint16_t)(1UL << pin));
}

const gpio_if_t gpio_stm32f1 = {
    .init   = port_gpio_init,
    .set    = port_gpio_set,
    .read   = port_gpio_read,
    .deinit = port_gpio_deinit,
};
