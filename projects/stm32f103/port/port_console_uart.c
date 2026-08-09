/**
 * @file port_console_uart.c
 * @brief STM32F103 Console UART (USART1 = log port PA9/PA10)
 */

#include "../hal_if_defines.h"
#include "port.h"
#include "main.h"

extern UART_HandleTypeDef huart1;

static int console_uart_init(uint32_t baudrate)
{
    (void)baudrate;
    return 0;
}

static void console_uart_deinit(void) {}

static int console_uart_write(const uint8_t *data, uint32_t size)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)data, size, 100);
    return (int)size;
}

static int console_uart_read(uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    (void)data; (void)size; (void)timeout_ms;
    return 0;
}

static void console_uart_register_rx_cb(void (*callback)(const uint8_t *data, uint32_t size))
{
    (void)callback;
}

const uart_if_t console_uart_stm32f1 = {
    .init              = console_uart_init,
    .deinit            = console_uart_deinit,
    .write             = console_uart_write,
    .read              = console_uart_read,
    .register_rx_cb    = console_uart_register_rx_cb,
    .instance          = 1,
};
