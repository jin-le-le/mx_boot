/**
 * @file port_gpio.c
 * @brief HC32F460 GPIO interface
 */

#include "../hal_if_defines.h"
#include "port.h"
#include <string.h>

static int port_gpio_init(uint8_t port, uint16_t pin, uint8_t mode, uint8_t pupd)
{
    stc_gpio_init_t stcGpioInit;

    (void)GPIO_StructInit(&stcGpioInit);

    /* mode */
    if (mode == BL_GPIO_MODE_IN) {
        stcGpioInit.u16PinDir = PIN_DIR_IN;
    } else if (mode == BL_GPIO_MODE_OUT) {
        stcGpioInit.u16PinDir        = PIN_DIR_OUT;
        stcGpioInit.u16PinOutputType = PIN_OUT_TYPE_CMOS;
    } else {
        /* AF / ANALOG not used by the bootloader */
        return -1;
    }

    /* pupd (HC32 only has internal pull-up, no pull-down) */
    if (pupd == GPIO_PUPD_PULLUP) {
        stcGpioInit.u16PullUp = PIN_PU_ON;
    } else {
        stcGpioInit.u16PullUp = PIN_PU_OFF;
    }

    return GPIO_Init(port, pin, &stcGpioInit);
}

static void port_gpio_set(uint8_t port, uint16_t pin, bool level)
{
    if (level) {
        GPIO_SetPins(port, pin);
    } else {
        GPIO_ResetPins(port, pin);
    }
}

static bool port_gpio_read(uint8_t port, uint16_t pin)
{
    return (GPIO_ReadInputPins(port, pin) == PIN_SET) ? true : false;
}

static void port_gpio_deinit(uint8_t port, uint16_t pin)
{
    (void)port; (void)pin;
    /* GPIO_DeInit has no equivalent in HC32 DDL; omitted */
}

const gpio_if_t gpio_hc32f4 = {
    .init   = port_gpio_init,
    .set    = port_gpio_set,
    .read   = port_gpio_read,
    .deinit = port_gpio_deinit,
};
