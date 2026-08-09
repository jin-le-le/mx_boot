/**
 * @file port_system.c
 * @brief HC32F460 System interface (Reset / VTOR / Jump / IRQ)
 *
 * Cortex-M4 core; NVIC/SysTick/SCB register layout is identical to STM32F4
 */

#include "../hal_if_defines.h"
#include "port.h"

/* Number of NVIC ICER registers: Cortex-M4 has 8 (HC32F460 has many interrupt sources, but the ICER register layout follows the core standard) */
#define CORTEX_M4_ICER_COUNT   8

static void port_system_reset(void)
{
    /* HC32F460 uses the RMU (Reset Management Unit) to trigger a software reset */
    NVIC_SystemReset();
}

static void port_system_set_vtor(uint32_t addr)
{
    SCB->VTOR = addr;
    __DSB();
}

static void port_system_disable_irq(uint8_t irq_num)
{
    NVIC_DisableIRQ((IRQn_Type)irq_num);
}

static void port_system_enable_irq(uint8_t irq_num)
{
    NVIC_EnableIRQ((IRQn_Type)irq_num);
}

static void port_system_get_unique_id(uint8_t *id)
{
    /* HC32F460 UID is read via EFM_GetUID */
    stc_efm_unique_id_t stcUID;
    EFM_GetUID(&stcUID);

    /* Copy out the 12-byte UID */
    id[0]  = (uint8_t)(stcUID.u32UniqueID0);
    id[1]  = (uint8_t)(stcUID.u32UniqueID0 >> 8);
    id[2]  = (uint8_t)(stcUID.u32UniqueID0 >> 16);
    id[3]  = (uint8_t)(stcUID.u32UniqueID0 >> 24);
    id[4]  = (uint8_t)(stcUID.u32UniqueID1);
    id[5]  = (uint8_t)(stcUID.u32UniqueID1 >> 8);
    id[6]  = (uint8_t)(stcUID.u32UniqueID1 >> 16);
    id[7]  = (uint8_t)(stcUID.u32UniqueID1 >> 24);
    id[8]  = (uint8_t)(stcUID.u32UniqueID2);
    id[9]  = (uint8_t)(stcUID.u32UniqueID2 >> 8);
    id[10] = (uint8_t)(stcUID.u32UniqueID2 >> 16);
    id[11] = (uint8_t)(stcUID.u32UniqueID2 >> 24);
}

static void port_system_disable_all_irq(void)
{
    /* Cortex-M4 standard: clear all ICER to disable all maskable interrupts */
    for (int i = 0; i < CORTEX_M4_ICER_COUNT; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFU;
    }
    /* Clear all pending */
    for (int i = 0; i < CORTEX_M4_ICER_COUNT; i++) {
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }
    /* Disable SysTick */
    SysTick->CTRL = 0;
}

static void port_system_jump(uint32_t sp, uint32_t pc)
{
    /* Cortex-M4 standard jump-to-app sequence:
     * 1. Set MSP = sp
     * 2. Jump to pc (pc must be a Thumb address, bit 0 = 1)
     */
    __set_MSP(sp);

    /* Jump */
    void (*app_entry)(void) = (void (*)(void))(pc);
    app_entry();

    /* Never returns */
    while (1) {}
}

const system_if_t system_hc32f4 = {
    .reset            = port_system_reset,
    .set_vtor         = port_system_set_vtor,
    .disable_irq      = port_system_disable_irq,
    .enable_irq       = port_system_enable_irq,
    .get_unique_id    = port_system_get_unique_id,
    .jump             = port_system_jump,
    .disable_all_irq  = port_system_disable_all_irq,
};
