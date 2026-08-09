/**
 * @file    main_usart4_echo.c
 * @brief   USART4 + PE6/PB9 Echo Test (FUNC already known: 36/37)
 *
 * Wiring (important, rewire required):
 *   USB-TTL.RX  --  PE6   (MCU USART4_TX)
 *   USB-TTL.TX  --  PB9   (MCU USART4_RX)
 *   USB-TTL.GND --  GND
 *
 * Test: user types characters in the serial terminal, the MCU echoes them back immediately.
 * Pass = the bootloader protocol port can use USART4.
 */

#include "hc32_ll.h"
#include <stdio.h>
#include <stdbool.h>

/* LED (PE6 is taken by USART4_TX, only PA7 remains) */
#define LED1_PORT           (GPIO_PORT_A)
#define LED1_PIN            (GPIO_PIN_07)

/* USART4 configuration (DDL official defaults, FUNC already known) */
#define PRINT_USART_UNIT         (CM_USART4)
#define PRINT_USART_FCG_ENABLE() (FCG_Fcg1PeriphClockCmd(FCG1_PERIPH_USART4, ENABLE))

#define PRINT_USART_TX_PORT      (GPIO_PORT_E)
#define PRINT_USART_TX_PIN       (GPIO_PIN_06)
#define PRINT_USART_TX_FUNC      (GPIO_FUNC_36)   /* Known USART4_TX */

#define PRINT_USART_RX_PORT      (GPIO_PORT_B)
#define PRINT_USART_RX_PIN       (GPIO_PIN_09)
#define PRINT_USART_RX_FUNC      (GPIO_FUNC_37)   /* Known USART4_RX */

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

static void USART_Print_Init(void)
{
    stc_usart_uart_init_t stcUartInit;
    float32_t f32Error;
    uint32_t u32Div;

    GPIO_SetFunc(PRINT_USART_TX_PORT, PRINT_USART_TX_PIN, PRINT_USART_TX_FUNC);
    GPIO_SetFunc(PRINT_USART_RX_PORT, PRINT_USART_RX_PIN, PRINT_USART_RX_FUNC);

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

    USART_FuncCmd(PRINT_USART_UNIT, USART_TX | USART_RX, ENABLE);
}

int32_t main(void)
{
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU |
                 LL_PERIPH_EFM | LL_PERIPH_SRAM);

    LED_Init();
    USART_Print_Init();

    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU |
                 LL_PERIPH_EFM | LL_PERIPH_SRAM);

    UART_Puts("\r\n");
    UART_Puts("========================================\r\n");
    UART_Puts("  HC32F460 - USART4 Echo Test\r\n");
    UART_Puts("  TX: PE6 FUNC=36\r\n");
    UART_Puts("  RX: PB9 FUNC=37\r\n");
    UART_Puts("  115200 8N1\r\n");
    UART_Puts("========================================\r\n");
    UART_Puts("Send any character; the MCU echoes it back as-is\r\n\r\n");

    uint32_t counter = 0;
    while (1) {
        /* Poll RX */
        if (SET == USART_GetStatus(PRINT_USART_UNIT, USART_FLAG_RX_FULL)) {
            uint16_t rx = USART_ReadData(PRINT_USART_UNIT);
            UART_PutChar((char)rx);
            if (rx == '\r') UART_PutChar('\n');
            GPIO_TogglePins(LED1_PORT, LED1_PIN);
        }

        /* Heartbeat */
        if ((counter % 50) == 0) {
            char buf[40];
            snprintf(buf, sizeof(buf), "[heartbeat] tick=%lu\r\n", counter);
            UART_Puts(buf);
        }

        counter++;
        DDL_DelayMS(100);
    }

    return 0;
}
