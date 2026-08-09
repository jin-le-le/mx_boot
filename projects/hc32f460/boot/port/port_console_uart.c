/**
 * @file port_console_uart.c
 * @brief HC32F460 Console UART (USART1 = log port PA10 TX)
 *
 * Verified: app_printf/main.c passed USART1 + FUNC=32 + 115200 printf output
 */

#include "../hal_if_defines.h"
#include "port.h"
#include "board_config.h"

/* USART1 init flag (avoid re-initialization) */
static bool s_console_inited = false;

static void USART1_Hw_Init(uint32_t baudrate)
{
    stc_usart_uart_init_t stcUartInit;
    float32_t f32Error;
    uint32_t u32Div;

    /* Configure PA10 as USART1_TX */
    GPIO_SetFunc(CONSOLE_TX_PORT, CONSOLE_TX_PIN, CONSOLE_TX_FUNC);

    /* Enable USART1 clock */
    FCG_Fcg1PeriphClockCmd(CONSOLE_USART_FCG, ENABLE);

    /* UART mode */
    (void)USART_UART_StructInit(&stcUartInit);
    stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    stcUartInit.u32DataWidth     = USART_DATA_WIDTH_8BIT;
    stcUartInit.u32Parity        = USART_PARITY_NONE;
    stcUartInit.u32StopBit       = USART_STOPBIT_1BIT;
    (void)USART_UART_Init(CONSOLE_USART_UNIT, &stcUartInit, NULL);

    /* Iterate CLK_DIV to find the optimal baud rate (official BSP style) */
    for (u32Div = 0UL; u32Div <= USART_CLK_DIV64; u32Div++) {
        USART_SetClockDiv(CONSOLE_USART_UNIT, u32Div);
        int32_t ret = USART_SetBaudrate(CONSOLE_USART_UNIT, baudrate, &f32Error);
        if ((LL_OK == ret) && (f32Error >= -0.025F) && (f32Error <= 0.025F)) {
            USART_FuncCmd(CONSOLE_USART_UNIT, USART_TX, ENABLE);
            break;
        }
    }

    s_console_inited = true;
}

static int console_uart_init(uint32_t baudrate)
{
    if (!s_console_inited) {
        USART1_Hw_Init(baudrate);
    }
    return 0;
}

static void console_uart_deinit(void)
{
    USART_FuncCmd(CONSOLE_USART_UNIT, USART_TX, DISABLE);
}

static int console_uart_write(const uint8_t *data, uint32_t size)
{
    if (!s_console_inited) return 0;

    for (uint32_t i = 0; i < size; i++) {
        uint32_t timeout = 50000U;
        while (RESET == USART_GetStatus(CONSOLE_USART_UNIT, USART_FLAG_TX_EMPTY)) {
            if (--timeout == 0U) break;
        }
        USART_WriteData(CONSOLE_USART_UNIT, (uint16_t)data[i]);
    }
    return (int)size;
}

static int console_uart_read(uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    (void)data; (void)size; (void)timeout_ms;
    return 0;  /* Console port does not read */
}

static void console_uart_register_rx_cb(void (*callback)(const uint8_t *data, uint32_t size))
{
    (void)callback;  /* Console port does not receive */
}

const uart_if_t console_uart_hc32f4 = {
    .init              = console_uart_init,
    .deinit            = console_uart_deinit,
    .write             = console_uart_write,
    .read              = console_uart_read,
    .register_rx_cb    = console_uart_register_rx_cb,
    .instance          = BL_CONSOLE_UART_INSTANCE,
};
