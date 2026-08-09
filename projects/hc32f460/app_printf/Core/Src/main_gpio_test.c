/**
 * @file    main_gpio_test.c
 * @brief   PA10 Direct GPIO Toggle Test - Verify Hardware Connection
 *
 * Purpose: configure PA10 as a plain GPIO output and toggle it every 500ms.
 *       - If a multimeter on PA10 shows 0V/3.3V switching, hardware is OK and the problem is in USART configuration.
 *       - If PA10 stays at 3.3V or 0V all the time, it is a hardware issue (PCB design or pin not broken out).
 *
 * Usage: temporarily rename main.c to main.c.bak, then rename this file to main.c.
 */

#include "hc32_ll.h"

/* LED */
#define LED1_PORT           (GPIO_PORT_A)
#define LED1_PIN            (GPIO_PIN_07)

/* Test GPIO (PA10) */
#define TEST_PORT           (GPIO_PORT_A)
#define TEST_PIN            (GPIO_PIN_10)

/* ICG region (must be placed at 0x400) */
#if defined (__GNUC__) && !defined (__CC_ARM)
const uint32_t u32ICG[] __attribute__((section(".icg_sec"))) =
#else
#error "Only GCC supported"
#endif
{
    0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL,
    0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL, 0xFFFFFFFFUL,
};

int32_t main(void)
{
    stc_gpio_init_t stcGpioInit;

    /* 1. Unlock peripherals */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU |
                 LL_PERIPH_EFM | LL_PERIPH_SRAM);

    /* 2. Configure LED1 as output */
    (void)GPIO_StructInit(&stcGpioInit);
    stcGpioInit.u16PinState       = PIN_STAT_RST;
    stcGpioInit.u16PinDir         = PIN_DIR_OUT;
    stcGpioInit.u16PinOutputType  = PIN_OUT_TYPE_CMOS;
    (void)GPIO_Init(LED1_PORT, LED1_PIN, &stcGpioInit);

    /* 3. Configure PA10 as output (key test) */
    (void)GPIO_Init(TEST_PORT, TEST_PIN, &stcGpioInit);

    /* 4. Lock peripherals back */
    LL_PERIPH_WP(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU |
                 LL_PERIPH_EFM | LL_PERIPH_SRAM);

    /* 5. Main loop: toggle LED and PA10 together */
    while (1) {
        GPIO_TogglePins(LED1_PORT, LED1_PIN);
        GPIO_TogglePins(TEST_PORT, TEST_PIN);
        DDL_DelayMS(500);
    }

    return 0;
}
