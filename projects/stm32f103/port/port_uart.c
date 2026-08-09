/**
 * @file port_uart.c
 * @brief STM32F103 UART interface (USART3 = protocol port)
 */

#include "../hal_if_defines.h"
#include "port.h"
#include "main.h"
#include "usart.h"
#include "board_config.h"

extern UART_HandleTypeDef huart3;
static void (*s_rx_callback)(const uint8_t *data, uint32_t size) = NULL;

/* F103 USART3 RX interrupt handler (called from CubeMX-generated stm32f4xx_it.c) */
static uint8_t s_rx_byte;

void uart3_register_rx_callback(void (*callback)(const uint8_t *data, uint32_t size))
{
    s_rx_callback = callback;
    HAL_UART_Receive_IT(&huart3, &s_rx_byte, 1);
}

/* HAL callback: fired on every received byte */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART3 && s_rx_callback) {
        s_rx_callback(&s_rx_byte, 1);
    }
    HAL_UART_Receive_IT(&huart3, &s_rx_byte, 1);
}

static int port_uart_init(uint32_t baudrate)
{
    (void)baudrate;
    s_rx_callback = NULL;
    uart3_register_rx_callback(NULL);
    /* Re-register to trigger the first RX */
    HAL_UART_Receive_IT(&huart3, &s_rx_byte, 1);
    return 0;
}

static void port_uart_deinit(void)
{
    HAL_UART_DeInit(&huart3);
}

static int port_uart_write(const uint8_t *data, uint32_t size)
{
    HAL_UART_Transmit(&huart3, (uint8_t *)data, size, HAL_MAX_DELAY);
    return (int)size;
}

static int port_uart_read(uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    (void)timeout_ms;
    HAL_StatusTypeDef status = HAL_UART_Receive(&huart3, data, size, timeout_ms);
    return (status == HAL_OK) ? (int)size : 0;
}

static void port_uart_register_rx_cb(void (*callback)(const uint8_t *data, uint32_t size))
{
    s_rx_callback = callback;
}

const uart_if_t uart_stm32f1 = {
    .init              = port_uart_init,
    .deinit            = port_uart_deinit,
    .write             = port_uart_write,
    .read              = port_uart_read,
    .register_rx_cb    = port_uart_register_rx_cb,
    .instance          = BL_UART_INSTANCE,
};
