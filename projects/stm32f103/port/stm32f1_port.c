/**
 * @file stm32f1_port.c
 * @brief Platform aggregation for STM32F103
 */

#include "../hal_if_defines.h"

extern const flash_if_t   flash_stm32f1;
extern const uart_if_t    uart_stm32f1;
extern const uart_if_t    console_uart_stm32f1;
extern const gpio_if_t    gpio_stm32f1;
extern const timer_if_t   timer_stm32f1;
extern const system_if_t  system_stm32f1;

static const platform_desc_t s_platform = {
    .flash         = &flash_stm32f1,
    .uart          = &uart_stm32f1,
    .console_uart  = &console_uart_stm32f1,
    .gpio          = &gpio_stm32f1,
    .timer         = &timer_stm32f1,
    .system        = &system_stm32f1,

    .chip_name  = "STM32F103C8T6",
    .chip_id    = 0x412,
    .bl_version = "1.0.0",
};

const platform_desc_t *platform_get(void)
{
    return &s_platform;
}

int platform_init(const platform_desc_t *desc)
{
    (void)desc;
    return 0;
}
