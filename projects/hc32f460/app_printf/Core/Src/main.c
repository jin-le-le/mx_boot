/**
 * @file    main.c
 * @brief   HC32F460PETB App Printf Project (Final Version)
 *
 * Hardware confirmation (located via FUNC scanning):
 *   - System XTAL clock = 8MHz (HRC not enabled by default)
 *   - USART1_TX on PA10 corresponds to FUNC=32 (HC32 pin-independent FUNC mapping)
 *   - USART1_RX on PA11 corresponds to FUNC=32 (same FUNC number)
 *
 * History:
 *   - Software-simulated UART verified the hardware link (clk=8000000)
 *   - Multi-USART + multi-FUNC scan found the correct combination (USART1+FUNC=32)
 */

#include "hc32_ll.h"
#include <stdio.h>

/*==============================================================================
 * LED Configuration
 *============================================================================*/
#define LED1_PORT           (GPIO_PORT_A)
#define LED1_PIN            (GPIO_PIN_07)   /* PA7 */
#define LED2_PORT           (GPIO_PORT_E)
#define LED2_PIN            (GPIO_PIN_06)   /* PE6 (spare, not lit) */

/*==============================================================================
 * USART1 Configuration (115200 8N1)
 * PA10 = USART1_TX, FUNC=32 (verified via scan)
 * PA11 = USART1_RX, FUNC=32 (assumed same as TX, unused)
 *============================================================================*/
#define PRINT_USART_UNIT         (CM_USART1)
#define PRINT_USART_FCG_ENABLE() (FCG_Fcg1PeriphClockCmd(FCG1_PERIPH_USART1, ENABLE))

#define PRINT_USART_TX_PORT      (GPIO_PORT_A)
#define PRINT_USART_TX_PIN       (GPIO_PIN_10)
#define PRINT_USART_TX_FUNC      (GPIO_FUNC_32)

#define PRINT_USART_RX_PORT      (GPIO_PORT_A)
#define PRINT_USART_RX_PIN       (GPIO_PIN_11)
#define PRINT_USART_RX_FUNC      (GPIO_FUNC_32)

#define PRINT_BAUDRATE           (115200UL)

/*==============================================================================
 * The App runs from 0x0000E000 and does not need ICG (ICG is the 0x400 region
 * belonging to the bootloader)
 *============================================================================*/

/*==============================================================================
 * Peripheral Initialization
 *============================================================================*/

static void LED_Init(void)
{
    stc_gpio_init_t stcGpioInit;

    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinState       = PIN_STAT_RST;
    stcGpioInit.u16PinDir         = PIN_DIR_OUT;
    stcGpioInit.u16PinOutputType  = PIN_OUT_TYPE_CMOS;
    (void)GPIO_Init(LED1_PORT, LED1_PIN, &stcGpioInit);

    GPIO_SetPins(LED1_PORT, LED1_PIN);  /* Turn off */
}

/* USART initialization (loop through CLK_DIV to find the optimal baud rate, following BSP official style) */
static void USART_Print_Init(void)
{
    stc_usart_uart_init_t stcUartInit;
    float32_t f32Error;
    uint32_t u32Div;

    /* Configure PA10 / PA11 as USART1 alternate function */
    GPIO_SetFunc(PRINT_USART_TX_PORT, PRINT_USART_TX_PIN, PRINT_USART_TX_FUNC);
    GPIO_SetFunc(PRINT_USART_RX_PORT, PRINT_USART_RX_PIN, PRINT_USART_RX_FUNC);

    /* Enable USART1 clock */
    PRINT_USART_FCG_ENABLE();

    /* USART UART mode */
    (void)USART_UART_StructInit(&stcUartInit);
    stcUartInit.u32OverSampleBit = USART_OVER_SAMPLE_8BIT;
    stcUartInit.u32DataWidth     = USART_DATA_WIDTH_8BIT;
    stcUartInit.u32Parity        = USART_PARITY_NONE;
    stcUartInit.u32StopBit       = USART_STOPBIT_1BIT;
    (void)USART_UART_Init(PRINT_USART_UNIT, &stcUartInit, NULL);

    /* Loop through CLK_DIV to find one with baud-rate error <= 2.5% */
    for (u32Div = 0UL; u32Div <= USART_CLK_DIV64; u32Div++) {
        USART_SetClockDiv(PRINT_USART_UNIT, u32Div);
        int32_t ret = USART_SetBaudrate(PRINT_USART_UNIT, PRINT_BAUDRATE, &f32Error);
        if ((LL_OK == ret) && (f32Error >= -0.025F) && (f32Error <= 0.025F)) {
            USART_FuncCmd(PRINT_USART_UNIT, USART_TX, ENABLE);
            break;
        }
    }
}

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

/*==============================================================================
 * Main
 *============================================================================*/
int32_t main(void)
{
    /* 1. Unlock peripherals */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU |
                 LL_PERIPH_EFM | LL_PERIPH_SRAM);

    /* 2. SystemInit was already called in startup */

    /* 3. LED initialization */
    LED_Init();

    /* 4. USART1 initialization (115200 8N1) */
    USART_Print_Init();

    /* 5. Lock peripherals back */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU |
                 LL_PERIPH_EFM | LL_PERIPH_SRAM);

    /* 6. Boot banner */
    UART_Puts("\r\n");
    UART_Puts("========================================\r\n");
    UART_Puts("  HC32F460PETB - App Printf Demo\r\n");
    UART_Puts("  USART1 @ PA10/PA11 FUNC=32\r\n");
    UART_Puts("  115200 8N1, SystemCoreClock=");
    {
        char buf[20];
        snprintf(buf, sizeof(buf), "%lu", SystemCoreClock);
        UART_Puts(buf);
    }
    UART_Puts("\r\n");
    UART_Puts("========================================\r\n");

    /* 7. Main loop */
    uint32_t counter = 0;
    char buf[64];

    while (1) {
        GPIO_TogglePins(LED1_PORT, LED1_PIN);

        if ((counter % 10) == 0) {
            snprintf(buf, sizeof(buf), "Hello HC32 - tick=%lu\r\n", counter);
            UART_Puts(buf);
        }

        counter++;
        DDL_DelayMS(100);
    }

    return 0;
}

/*==============================================================================
 * newlib system call retarget
 *============================================================================*/
#if defined (__GNUC__) && !defined (__CC_ARM)
extern int _write(int fd, char *ptr, int len);
int _write(int fd, char *ptr, int len)
{
    (void)fd;
    for (int i = 0; i < len; i++) {
        if (ptr[i] == '\n') {
            uint32_t timeout = 50000U;
            while (RESET == USART_GetStatus(PRINT_USART_UNIT, USART_FLAG_TX_EMPTY)) {
                if (--timeout == 0U) break;
            }
            USART_WriteData(PRINT_USART_UNIT, (uint16_t)'\r');
        }
        uint32_t timeout = 50000U;
        while (RESET == USART_GetStatus(PRINT_USART_UNIT, USART_FLAG_TX_EMPTY)) {
            if (--timeout == 0U) break;
        }
        USART_WriteData(PRINT_USART_UNIT, (uint16_t)ptr[i]);
    }
    return len;
}
#endif
