/**
 * @file    main_echo.c
 * @brief   USART1 RX Path Test - Echo Mode
 *
 * Known: USART1_TX is on PA10 with FUNC=32
 * Inferred: USART1_RX is on PA11 with FUNC=32 (usually the same number, to be verified)
 *
 * Test procedure:
 *   - Send any character from the serial terminal
 *   - The MCU immediately transmits it back from TX
 *   - Seeing the echo in the serial terminal means the RX path is OK
 *
 * Wiring:
 *   USB-TTL.RX  --  MCU.PA10  (TX)
 *   USB-TTL.TX  --  MCU.PA11  (RX)
 *   USB-TTL.GND --  MCU.GND
 */

#include "hc32_ll.h"
#include <stdio.h>

/* LED */
#define LED1_PORT           (GPIO_PORT_A)
#define LED1_PIN            (GPIO_PIN_07)

/* USART1 configuration (same as main.c, already verified) */
#define PRINT_USART_UNIT         (CM_USART1)
#define PRINT_USART_FCG_ENABLE() (FCG_Fcg1PeriphClockCmd(FCG1_PERIPH_USART1, ENABLE))

#define PRINT_USART_TX_PORT      (GPIO_PORT_A)
#define PRINT_USART_TX_PIN       (GPIO_PIN_10)
#define PRINT_USART_TX_FUNC      (GPIO_FUNC_32)

#define PRINT_USART_RX_PORT      (GPIO_PORT_A)
#define PRINT_USART_RX_PIN       (GPIO_PIN_11)
#define PRINT_USART_RX_FUNC      (GPIO_FUNC_32)   /* Assumed same as TX */

#define PRINT_BAUDRATE           (115200UL)

/* ICG */
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
    stc_irq_signin_config_t stcIrqSignin;

    /* Configure PA10/PA11 */
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

    /* Register the RX interrupt (USART1 RI = receive complete) */
    stcIrqSignin.enIRQn    = INT000_IRQn;
    stcIrqSignin.enIntSrc  = INT_SRC_USART1_RI;
    stcIrqSignin.pfnCallback = NULL;  /* Polling mode; the RX flag is checked in main below */
    (void)INTC_IrqSignIn(&stcIrqSignin);
    NVIC_ClearPendingIRQ(stcIrqSignin.enIRQn);
    NVIC_SetPriority(stcIrqSignin.enIRQn, DDL_IRQ_PRIO_DEFAULT);
    NVIC_EnableIRQ(stcIrqSignin.enIRQn);

    /* Enable TX + RX + RX interrupt */
    USART_FuncCmd(PRINT_USART_UNIT, USART_TX | USART_RX | USART_INT_RX, ENABLE);
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
    UART_Puts("  HC32F460PETB - USART1 Echo Test\r\n");
    UART_Puts("  TX: PA10 FUNC=32\r\n");
    UART_Puts("  RX: PA11 FUNC=32 (assumed)\r\n");
    UART_Puts("  115200 8N1\r\n");
    UART_Puts("========================================\r\n");
    UART_Puts("Send any character; the MCU echoes it back as-is\r\n");
    UART_Puts("(If the echo works, the RX path is OK)\r\n\r\n");

    uint32_t counter = 0;
    while (1) {
        /* Poll the RX flag (no interrupt dependency; simpler and more reliable) */
        if (SET == USART_GetStatus(PRINT_USART_UNIT, USART_FLAG_RX_FULL)) {
            uint16_t rx = USART_ReadData(PRINT_USART_UNIT);

            /* Echo back */
            UART_PutChar((char)rx);

            /* On receiving a carriage return, add a line feed for nicer display */
            if (rx == '\r') {
                UART_PutChar('\n');
            }

            /* Toggle the LED to indicate data received */
            GPIO_TogglePins(LED1_PORT, LED1_PIN);
        }

        /* Print a heartbeat every 5 seconds (avoid complete silence) */
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
