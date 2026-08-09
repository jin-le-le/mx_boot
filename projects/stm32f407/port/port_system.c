/**
 * @file port_system.c
 * @brief System interface implementation
 *
 * Jump-to-app implementation notes:
 *   No longer uses a separate assembly file (jumpapp.s); instead it uses
 *   standard CMSIS functions + a function pointer. __set_MSP() sets the stack
 *   pointer and the function-pointer jump keeps Thumb mode, portable across toolchains.
 */

#include "../hal_if_defines.h"
#include "port.h"
#include "main.h"
#include <string.h>

static void port_system_reset(void)
{
    HAL_NVIC_SystemReset();
}

static void port_system_set_vtor(uint32_t addr)
{
    SCB->VTOR = addr;
}

static void port_system_disable_irq(uint8_t irq_num)
{
    HAL_NVIC_DisableIRQ((IRQn_Type)irq_num);
}

static void port_system_enable_irq(uint8_t irq_num)
{
    HAL_NVIC_EnableIRQ((IRQn_Type)irq_num);
}

/**
 * Cortex-M4 (STM32F4) implementation:
 *   1. __disable_irq() - disable global IRQ (PRIMASK=1)
 *   2. Clear ICER[0..7] - clear all interrupt enable bits (up to 256 IRQs)
 *   3. Clear ICPR[0..7] - clear all pending interrupts
 *
 * Cortex-M3 (STM32F1) is identical to M4 and can reuse this code.
 * Cortex-M0/M0+ has only 1 ICER/ICPR (up to 32 IRQs); the port layer
 * must change this to write only [0].
 */
static void port_system_disable_all_irq(void)
{
    __disable_irq();
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFU;
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }
}

static void port_system_get_unique_id(uint8_t *id)
{
    uint32_t *uid = (uint32_t *)0x1FFF7A10;
    id[0] = (uint8_t)(uid[0] >> 0);
    id[1] = (uint8_t)(uid[0] >> 8);
    id[2] = (uint8_t)(uid[0] >> 16);
    id[3] = (uint8_t)(uid[0] >> 24);
    id[4] = (uint8_t)(uid[1] >> 0);
    id[5] = (uint8_t)(uid[1] >> 8);
    id[6] = (uint8_t)(uid[1] >> 16);
    id[7] = (uint8_t)(uid[1] >> 24);
    id[8]  = (uint8_t)(uid[2] >> 0);
    id[9]  = (uint8_t)(uid[2] >> 8);
    id[10] = (uint8_t)(uid[2] >> 16);
    id[11] = (uint8_t)(uid[2] >> 24);
    (void)uid;
}

/**
 * Jump to application: set SP and branch to PC.
 *
 * Uses CMSIS + a function pointer instead of an assembly file, portable
 * across toolchains (GCC/IAR/Keil/ARM CC).
 * Applicable to all ARM Cortex-M chips (M0/M0+/M3/M4/M7/M23/M33).
 *
 * Parameters:
 *   sp - new stack pointer (read from word 0 of the App vector table)
 *   pc - new PC (read from word 1 of the App vector table)
 *
 * Implementation notes:
 *   1. __set_MSP() - CMSIS standard, writes the MSP register directly
 *   2. pc | 0x1 - set the Thumb bit (Cortex-M supports only Thumb instructions)
 *   3. __DSB/__ISB - memory barriers, ensure SP write and instruction pipeline are consistent
 *   4. Function-pointer call - the compiler emits a BX instruction (never returns)
 */
static void port_system_jump(uint32_t sp, uint32_t pc)
{
    /* 1. Set stack pointer */
    __set_MSP(sp);

    /* 2. Function pointer + Thumb bit */
    typedef void (*app_entry_t)(void);
    app_entry_t app_entry = (app_entry_t)(pc | 0x1u);

    /* 3. Memory barriers to prevent pipeline reordering */
    __DSB();
    __ISB();

    /* 4. Jump, never returns */
    app_entry();
}

const system_if_t system_stm32f4 = {
    .reset          = port_system_reset,
    .set_vtor       = port_system_set_vtor,
    .disable_irq    = port_system_disable_irq,
    .enable_irq     = port_system_enable_irq,
    .get_unique_id  = port_system_get_unique_id,
    .jump           = port_system_jump,
    .disable_all_irq = port_system_disable_all_irq,
};
