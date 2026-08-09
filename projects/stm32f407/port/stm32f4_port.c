/**
 * @file stm32f4_port.c
 * @brief Platform aggregation for STM32F4
 */

#include "../hal_if_defines.h"
#include "port.h"

extern const flash_if_t flash_stm32f4;
extern const uart_if_t uart_stm32f4;
extern const uart_if_t console_uart_stm32f4;
extern const gpio_if_t gpio_stm32f4;
extern const timer_if_t timer_stm32f4;
extern const system_if_t system_stm32f4;

static const platform_desc_t s_platform = {
    .flash         = &flash_stm32f4,
    .uart          = &uart_stm32f4,
    .console_uart  = &console_uart_stm32f4,
    .gpio          = &gpio_stm32f4,
    .timer         = &timer_stm32f4,
    .system        = &system_stm32f4,

    .chip_name = "STM32F407ZGTx",
    .chip_id   = 0x413,
    .bl_version = "1.0.0",
};

const platform_desc_t *platform_get(void)
{
    return &s_platform;
}

int platform_init(const platform_desc_t *desc)
{
    (void)desc;
    /* Clocks are already initialized by SystemClock_Config() in main.c */
    return 0;
}
