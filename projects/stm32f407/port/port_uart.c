/**
 * @file port_uart.c
 * @brief UART interface implementation using HAL
 */

#include "../hal_if_defines.h"
#include "port.h"
#include "main.h"
#include <string.h>

extern UART_HandleTypeDef huart3;
extern void uart3_register_rx_callback(void (*callback)(const uint8_t *data, uint32_t size));

static void (*s_rx_callback)(const uint8_t *data, uint32_t size) = NULL;

static void uart_rx_wrapper(const uint8_t *data, uint32_t size)
{
    if (s_rx_callback) {
        s_rx_callback(data, size);
    }
}

static int port_uart_init(uint32_t baudrate)
{
    (void)baudrate;
    /* UART is already initialized by MX_USART3_UART_Init() */
    s_rx_callback = NULL;
    uart3_register_rx_callback(uart_rx_wrapper);
    return 0;
}

static void port_uart_deinit(void)
{
    HAL_UART_DeInit(&huart3);
}

static int port_uart_write(const uint8_t *data, uint32_t size)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)data, size, HAL_MAX_DELAY);
    return size;
}

static int port_uart_read(uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    (void)timeout_ms;
    HAL_StatusTypeDef status = HAL_UART_Receive(&huart3, data, size, timeout_ms);
    if (status == HAL_OK) {
        return size;
    }
    return 0;
}

static void port_uart_register_rx_cb(void (*callback)(const uint8_t *data, uint32_t size))
{
    s_rx_callback = callback;
}

const uart_if_t uart_stm32f4 = {
    .init              = port_uart_init,
    .deinit            = port_uart_deinit,
    .write             = port_uart_write,
    .read              = port_uart_read,
    .register_rx_cb    = port_uart_register_rx_cb,
    .instance          = BL_UART_INSTANCE,
};
