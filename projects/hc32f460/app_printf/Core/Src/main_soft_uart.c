/**
 * @file    main_soft_uart.c
 * @brief   PA10 Software-simulated UART TX (SysTick precise-delay version)
 *
 * Completely bypasses the USART peripheral; uses pure GPIO + SysTick precise delays.
 *
 * Key improvement: uses SystemCoreClock to compute the bit time dynamically,
 * so the baud rate is correct whether HRC is 16MHz or 20MHz.
 */

#include "hc32_ll.h"
#include <stdio.h>

/* LED */
#define LED1_PORT           (GPIO_PORT_A)
#define LED1_PIN            (GPIO_PIN_07)

/* Software UART TX pin: PA10 */
#define SOFT_TX_PORT        (GPIO_PORT_A)
#define SOFT_TX_PIN         (GPIO_PIN_10)
#define SOFT_TX_HIGH()      GPIO_SetPins(SOFT_TX_PORT, SOFT_TX_PIN)
#define SOFT_TX_LOW()       GPIO_ResetPins(SOFT_TX_PORT, SOFT_TX_PIN)

#define SOFT_BAUDRATE       9600UL    /* Test at 9600 first; bit time is more forgiving */

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

/* SysTick reload value: bit time = SystemCoreClock / baudrate cycles.
 * SysTick max is 24-bit (0xFFFFFF = 16M); at 115200 bps under 16MHz the reload is 139, no problem. */
static volatile uint32_t s_u32BitReload;

/* Configure SysTick for bit delays (no interrupt; pure polling of COUNTFLAG) */
static void SoftUART_SysTick_Init(void)
{
    /* SystemCoreClock is set by SystemCoreClockUpdate inside SystemInit, which is called in startup */
    s_u32BitReload = SystemCoreClock / SOFT_BAUDRATE;
    if (s_u32BitReload > 0x00FFFFFFU) {
        s_u32BitReload = 0x00FFFFFFU;  /* SysTick 24-bit limit */
    }

    SysTick->LOAD = s_u32BitReload;
    SysTick->VAL  = 0UL;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk | SysTick_CTRL_ENABLE_Msk;
    /* Do not enable SysTick_CTRL_TICKINT_Msk to avoid interrupts */
}

/* Wait for one bit time (poll the SysTick COUNTFLAG) */
static inline void SoftUART_DelayBit(void)
{
    /* Wait for COUNTFLAG to be set (set once every (LOAD+1) cycles) */
    while ((SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk) == 0U) {
    }
    /* Reading COUNTFLAG automatically clears it */
}

/* Software UART send one byte */
static void SoftUART_PutChar(uint8_t b)
{
    /* Start bit (pull low) */
    SOFT_TX_LOW();
    SysTick->VAL = 0UL;  /* Reset counter */
    SoftUART_DelayBit();

    /* 8 data bits (LSB first) */
    for (uint8_t i = 0; i < 8; i++) {
        if (b & 0x01U) {
            SOFT_TX_HIGH();
        } else {
            SOFT_TX_LOW();
        }
        SysTick->VAL = 0UL;
        SoftUART_DelayBit();
        b >>= 1;
    }

    /* Stop bit (pull high) + guard interval */
    SOFT_TX_HIGH();
    SysTick->VAL = 0UL;
    SoftUART_DelayBit();
    SoftUART_DelayBit();  /* Extra 1-bit guard */
}

/* Send a string */
static void SoftUART_Puts(const char *s)
{
    while (*s) {
        SoftUART_PutChar((uint8_t)*s);
        s++;
    }
}

int32_t main(void)
{
    stc_gpio_init_t stcGpioInit;

    /* 1. Unlock peripherals */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU |
                 LL_PERIPH_EFM | LL_PERIPH_SRAM);

    /* 2. Configure LED + PA10 as GPIO outputs */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinState       = PIN_STAT_RST;
    stcGpioInit.u16PinDir         = PIN_DIR_OUT;
    stcGpioInit.u16PinOutputType  = PIN_OUT_TYPE_CMOS;
    (void)GPIO_Init(LED1_PORT, LED1_PIN, &stcGpioInit);
    (void)GPIO_Init(SOFT_TX_PORT, SOFT_TX_PIN, &stcGpioInit);

    /* 3. Pull PA10 high (UART idle) */
    SOFT_TX_HIGH();

    /* 4. Lock peripherals back */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU |
                 LL_PERIPH_EFM | LL_PERIPH_SRAM);

    /* 5. Initialize SysTick (auto-computes bit time from SystemCoreClock) */
    SoftUART_SysTick_Init();

    /* 6. Main loop */
    uint32_t counter = 0;
    char buf[64];

    while (1) {
        GPIO_TogglePins(LED1_PORT, LED1_PIN);

        if ((counter % 10) == 0) {
            snprintf(buf, sizeof(buf), "Hello HC32 (soft uart) - tick=%lu clk=%lu\r\n",
                     counter, SystemCoreClock);
            SoftUART_Puts(buf);
        }

        counter++;
        DDL_DelayMS(100);
    }

    return 0;
}
