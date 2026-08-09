/**
 * @file    main_rx_scan.c
 * @brief   USART1 RX FUNC Auto Scan (loopback test by shorting PA10-PA11)
 *
 * Test procedure:
 *   1. After flashing, use a jumper wire to short PA10 and PA11 (MCU self loopback).
 *   2. The program cycles the FUNC number on PA11 (0-15, 32-59).
 *   3. Under each FUNC: the MCU transmits "FUNC=N", then immediately checks whether RX receives it.
 *   4. If received, printf reports "RX WORKS at FUNC=N".
 *   5. When the serial terminal shows "RX WORKS at FUNC=N", that N is the correct FUNC for PA11.
 *
 * Wiring (important):
 *   USB-TTL.RX  --  PA10  (printf only, same as before)
 *   Jumper wire --  short PA10 and PA11 (MCU self loopback)
 *   USB-TTL.GND --  GND
 *
 *   USB-TTL.TX is not needed this time (no need for the PC to send data).
 */

#include "hc32_ll.h"
#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#define LED1_PORT           (GPIO_PORT_A)
#define LED1_PIN            (GPIO_PIN_07)

#define PRINT_USART_UNIT         (CM_USART1)
#define PRINT_USART_FCG_ENABLE() (FCG_Fcg1PeriphClockCmd(FCG1_PERIPH_USART1, ENABLE))

#define PRINT_USART_TX_PORT      (GPIO_PORT_A)
#define PRINT_USART_TX_PIN       (GPIO_PIN_10)
#define PRINT_USART_TX_FUNC      (GPIO_FUNC_32)   /* Verified */

#define PRINT_USART_RX_PORT      (GPIO_PORT_A)
#define PRINT_USART_RX_PIN       (GPIO_PIN_11)
/* RX FUNC to be scanned */

#define PRINT_BAUDRATE           (115200UL)

#if defined (__GNUC__) && !defined (__CC_ARM)
const uint32_t u32ICG[] __attribute__((section(".icg_sec"))) =
#else
#error "Only GCC supported"
#endif
{
    0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL,
    0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL,
};

static void UART_PutChar(char c)
{
    uint32_t timeout = 50000U;
    while (RESET == USART_GetStatus(PRINT_USART_UNIT, USART_FLAG_TX_EMPTY)) {
        if (--timeout == 0U) return;
    }
    USART_WriteData(PRINT_USART_UNIT, (uint16_t)c);
}

static void UART_Puts(const char *s)
{
    while (*s) {
        if (*s == '\n') UART_PutChar('\r');
        UART_PutChar(*s++);
    }
}

static void LED_Init(void)
{
    stc_gpio_init_t stcGpioInit;
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinState       = PIN_STAT_RST;
    stcGpioInit.u16PinDir         = PIN_DIR_OUT;
    stcGpioInit.u16PinOutputType  = PIN_OUT_TYPE_CMOS;
    (void)GPIO_Init(LED1_PORT, LED1_PIN, &stcGpioInit);
    GPIO_SetPins(LED1_PORT, LED1_PIN);
}

/* Configure USART1 without configuring the RX pin (the RX pin's FUNC is switched dynamically in the main loop) */
static void USART_Hw_Init(void)
{
    stc_usart_uart_init_t stcUartInit;
    float32_t f32Error;
    uint32_t u32Div;

    /* Configure only the TX pin (fixed FUNC=32) */
    GPIO_SetFunc(PRINT_USART_TX_PORT, PRINT_USART_TX_PIN, PRINT_USART_TX_FUNC);

    PRINT_USART_FCG_ENABLE();

    (void)USART_UART_StructInit(&stcUartInit);
    stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    stcUartInit.u32DataWidth     = USART_DATA_WIDTH_8BIT;
    stcUartInit.u32Parity        = USART_PARITY_NONE;
    stcUartInit.u32StopBit       = USART_STOPBIT_1BIT;
    (void)USART_UART_Init(PRINT_USART_UNIT, &stcUartInit, NULL);

    for (u32Div = 0UL; u32Div <= USART_CLK_DIV64; u32Div++) {
        USART_SetClockDiv(PRINT_USART_UNIT, u32Div);
        int32_t ret = USART_SetBaudrate(PRINT_USART_UNIT, PRINT_BAUDRATE, &f32Error);
        if ((LL_OK == ret) && (f32Error >= -0.025F) && (f32Error <= 0.025F)) {
            break;
        }
    }

    /* Enable TX + RX */
    USART_FuncCmd(PRINT_USART_UNIT, USART_TX | USART_RX, ENABLE);
}

/* Test whether RX can receive the byte sent by TX under the specified FUNC */
static bool RX_FUNC_Test(uint32_t func)
{
    char tx_buf[32];
    char rx_buf[32] = {0};
    uint32_t rx_idx = 0;

    /* Switch PA11 FUNC */
    GPIO_SetFunc(PRINT_USART_RX_PORT, PRINT_USART_RX_PIN, (uint16_t)func);

    /* Drain the RX buffer (read DR until empty) */
    while (SET == USART_GetStatus(PRINT_USART_UNIT, USART_FLAG_RX_FULL)) {
        (void)USART_ReadData(PRINT_USART_UNIT);
    }

    /* Send the test string */
    snprintf(tx_buf, sizeof(tx_buf), "F%lu", func);
    UART_Puts(tx_buf);

    /* Wait for RX to receive (wait at most 50ms) */
    for (uint32_t wait = 0; wait < 5000; wait++) {
        while (SET == USART_GetStatus(PRINT_USART_UNIT, USART_FLAG_RX_FULL)) {
            if (rx_idx < sizeof(rx_buf) - 1) {
                rx_buf[rx_idx++] = (char)USART_ReadData(PRINT_USART_UNIT);
            } else {
                (void)USART_ReadData(PRINT_USART_UNIT);
            }
        }
        if (rx_idx >= strlen(tx_buf)) break;
        DDL_DelayMS(1);
    }

    /* Compare received content with sent content */
    if (rx_idx >= strlen(tx_buf) && memcmp(rx_buf, tx_buf, strlen(tx_buf)) == 0) {
        return true;
    }
    return false;
}

int32_t main(void)
{
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU |
                 LL_PERIPH_EFM | LL_PERIPH_SRAM);

    LED_Init();
    USART_Hw_Init();

    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU |
                 LL_PERIPH_EFM | LL_PERIPH_SRAM);

    UART_Puts("\r\n");
    UART_Puts("========================================\r\n");
    UART_Puts("  USART1 RX FUNC Auto Scan\r\n");
    UART_Puts("  Short PA10 and PA11 (jumper wire)\r\n");
    UART_Puts("  The program will auto-find the correct FUNC for PA11\r\n");
    UART_Puts("========================================\r\n\r\n");

    while (1) {
        bool found = false;

        for (uint32_t func = 0; func <= 60; func++) {
            if (func >= 16 && func <= 31) continue;

            /* Unlock GPIO (required by GPIO_SetFunc) */
            LL_PERIPH_WE(LL_PERIPH_GPIO);

            bool ok = RX_FUNC_Test(func);

            /* Lock GPIO back */
            LL_PERIPH_WP(LL_PERIPH_GPIO);

            char buf[64];
            if (ok) {
                snprintf(buf, sizeof(buf), "*** RX WORKS at FUNC=%lu ***\r\n", func);
                UART_Puts(buf);
                GPIO_ResetPins(LED1_PORT, LED1_PIN);  /* LED on = found */
                found = true;
            } else {
                snprintf(buf, sizeof(buf), "FUNC=%lu no\r\n", func);
                UART_Puts(buf);
                GPIO_TogglePins(LED1_PORT, LED1_PIN);
            }

            DDL_DelayMS(200);
        }

        if (found) {
            UART_Puts("\r\n=== Scan done, found at least one working FUNC ===\r\n");
            UART_Puts("=== Wait 5s, then scan again ===\r\n\r\n");
            DDL_DelayMS(5000);
        } else {
            UART_Puts("\r\n=== No FUNC works. Check PA10-PA11 jumper! ===\r\n\r\n");
            DDL_DelayMS(2000);
        }
    }

    return 0;
}
