/**
 * @file hc32f4_port.c
 * @brief HC32F460 platform descriptor (aggregation layer)
 *
 * Bundles the 5 port interfaces into platform_desc_t so boot/core can invoke the chip-specific implementation
 */

#include "../hal_if_defines.h"
#include "port.h"

extern const flash_if_t   flash_hc32f4;
extern const uart_if_t    uart_hc32f4;
extern const uart_if_t    console_uart_hc32f4;
extern const gpio_if_t    gpio_hc32f4;
extern const timer_if_t   timer_hc32f4;
extern const system_if_t  system_hc32f4;

const platform_desc_t platform_hc32f4 = {
    .flash         = &flash_hc32f4,
    .uart          = &uart_hc32f4,
    .console_uart  = &console_uart_hc32f4,
    .gpio          = &gpio_hc32f4,
    .timer         = &timer_hc32f4,
    .system        = &system_hc32f4,

    .chip_name     = "HC32F460PETB",
    .chip_id       = 0xF460,
    .bl_version    = "1.0.0",
};

const platform_desc_t *platform_get(void)
{
    return &platform_hc32f4;
}
