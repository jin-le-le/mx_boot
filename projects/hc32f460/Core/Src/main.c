/**
 * @file    main.c
 * @brief   HC32F460 Bootloader entry point
 */

#include "hc32_ll.h"
#include "bl_config.h"

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

int main(void)
{
    /* Unlock all peripheral registers (HC32-specific) */
    LL_PERIPH_WE(LL_PERIPH_GPIO | LL_PERIPH_FCG | LL_PERIPH_PWC_CLK_RMU |
                 LL_PERIPH_EFM | LL_PERIPH_SRAM);

    /* Bootloader Facade: initialize + run */
    boot_init();
    boot_run();

    /* Should never reach here */
    while (1) {
    }

    return 0;
}
