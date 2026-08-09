/**
 * @file port_uart.c
 * @brief HC32F460 Protocol UART (USART4 = upgrade port PE6 TX / PB9 RX)
 *
 * Verified: app_printf/main_usart4_echo.c passed USART4 + FUNC=36/37 + 115200 echo test
 *
 * RX uses interrupt reception to avoid overrun (modeled after the F103 port_uart.c style)
 */

#include "../hal_if_defines.h"
#include "port.h"
#include "board_config.h"
#include <string.h>

/* RX callback (registered by boot/core) */
static void (*s_rx_callback)(const uint8_t *data, uint32_t size) = NULL;

/* Single-byte buffer for RX interrupt */
static uint8_t s_rx_byte;

/* USART4 init flag */
static bool s_proto_inited = false;

/* USART4 RX interrupt handler */
static void USART4_RxCallback(void)
{
    if (SET == USART_GetStatus(PROTO_USART_UNIT, USART_FLAG_RX_FULL)) {
        s_rx_byte = (uint8_t)USART_ReadData(PROTO_USART_UNIT);
        if (s_rx_callback) {
            s_rx_callback(&s_rx_byte, 1);
        }
    }
    /* Clear error flags */
    USART_ClearStatus(PROTO_USART_UNIT, (USART_FLAG_PARITY_ERR | USART_FLAG_FRAME_ERR | USART_FLAG_OVERRUN));
}

static void USART4_Hw_Init(uint32_t baudrate)
{
    stc_usart_uart_init_t stcUartInit;
    stc_irq_signin_config_t stcIrqSignin;
    float32_t f32Error;
    uint32_t u32Div;

    /* Configure PE6 / PB9 as USART4 alternate-function pins */
    GPIO_SetFunc(PROTO_TX_PORT, PROTO_TX_PIN, PROTO_TX_FUNC);
    GPIO_SetFunc(PROTO_RX_PORT, PROTO_RX_PIN, PROTO_RX_FUNC);

    /* Enable USART4 clock */
    FCG_Fcg1PeriphClockCmd(PROTO_USART_FCG, ENABLE);

    /* UART mode */
    (void)USART_UART_StructInit(&stcUartInit);
    stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    stcUartInit.u32DataWidth     = USART_DATA_WIDTH_8BIT;
    stcUartInit.u32Parity        = USART_PARITY_NONE;
    stcUartInit.u32StopBit       = USART_STOPBIT_1BIT;
    (void)USART_UART_Init(PROTO_USART_UNIT, &stcUartInit, NULL);

    /* Iterate CLK_DIV to find the optimal baud rate */
    for (u32Div = 0UL; u32Div <= USART_CLK_DIV64; u32Div++) {
        USART_SetClockDiv(PROTO_USART_UNIT, u32Div);
        int32_t ret = USART_SetBaudrate(PROTO_USART_UNIT, baudrate, &f32Error);
        if ((LL_OK == ret) && (f32Error >= -0.025F) && (f32Error <= 0.025F)) {
            break;
        }
    }

    /* Register RX interrupt (USART4 RI = receive complete) */
    stcIrqSignin.enIRQn      = INT001_IRQn;
    stcIrqSignin.enIntSrc    = INT_SRC_USART4_RI;
    stcIrqSignin.pfnCallback = USART4_RxCallback;
    (void)INTC_IrqSignIn(&stcIrqSignin);
    NVIC_ClearPendingIRQ(stcIrqSignin.enIRQn);
    NVIC_SetPriority(stcIrqSignin.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqSignin.enIRQn);

    /* RX error interrupt (handle overrun) */
    stcIrqSignin.enIRQn      = INT002_IRQn;
    stcIrqSignin.enIntSrc    = INT_SRC_USART4_EI;
    stcIrqSignin.pfnCallback = USART4_RxCallback;
    (void)INTC_IrqSignIn(&stcIrqSignin);
    NVIC_ClearPendingIRQ(stcIrqSignin.enIRQn);
    NVIC_SetPriority(stcIrqSignin.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqSignin.enIRQn);

    /* Enable TX + RX + RX interrupt */
    USART_FuncCmd(PROTO_USART_UNIT, USART_TX | USART_RX | USART_INT_RX, ENABLE);

    s_proto_inited = true;
}

static int port_uart_init(uint32_t baudrate)
{
    if (!s_proto_inited) {
        USART4_Hw_Init(baudrate);
    }
    return 0;
}

static void port_uart_deinit(void)
{
    USART_FuncCmd(PROTO_USART_UNIT, USART_TX | USART_RX | USART_INT_RX, DISABLE);
}

static int port_uart_write(const uint8_t *data, uint32_t size)
{
    if (!s_proto_inited) return 0;

    for (uint32_t i = 0; i < size; i++) {
        uint32_t timeout = 50000U;
        while (RESET == USART_GetStatus(PROTO_USART_UNIT, USART_FLAG_TX_EMPTY)) {
            if (--timeout == 0U) break;
        }
        USART_WriteData(PROTO_USART_UNIT, (uint16_t)data[i]);
    }
    return (int)size;
}

static int port_uart_read(uint8_t *data, uint32_t size, uint32_t timeout_ms)
{
    (void)data; (void)size; (void)timeout_ms;
    return 0;  /* RX is handled by the interrupt callback; read is not implemented */
}

static void port_uart_register_rx_cb(void (*callback)(const uint8_t *data, uint32_t size))
{
    s_rx_callback = callback;
}

const uart_if_t uart_hc32f4 = {
    .init              = port_uart_init,
    .deinit            = port_uart_deinit,
    .write             = port_uart_write,
    .read              = port_uart_read,
    .register_rx_cb    = port_uart_register_rx_cb,
    .instance          = BL_UART_INSTANCE,
};
