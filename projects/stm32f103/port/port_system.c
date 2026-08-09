/**
 * @file port_system.c
 * @brief STM32F103 System interface
 *
 * CMSIS on Cortex-M3 is identical to M4; this code is essentially the same as F407.
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

static void port_system_get_unique_id(uint8_t *id)
{
    /* F103 UID: 0x1FFFF7E8 (3 words) */
    uint32_t *uid = (uint32_t *)0x1FFFF7E8;
    id[0]  = (uint8_t)(uid[0]);
    id[1]  = (uint8_t)(uid[0] >> 8);
    id[2]  = (uint8_t)(uid[0] >> 16);
    id[3]  = (uint8_t)(uid[0] >> 24);
    id[4]  = (uint8_t)(uid[1]);
    id[5]  = (uint8_t)(uid[1] >> 8);
    id[6]  = (uint8_t)(uid[1] >> 16);
    id[7]  = (uint8_t)(uid[1] >> 24);
    id[8]  = (uint8_t)(uid[2]);
    id[9]  = (uint8_t)(uid[2] >> 8);
    id[10] = (uint8_t)(uid[2] >> 16);
    id[11] = (uint8_t)(uid[2] >> 24);
}

/**
 * Cortex-M3: 8 ICER/ICPR (same as M4)
 */
static void port_system_disable_all_irq(void)
{
    __disable_irq();
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFU;
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }
}

static void port_system_jump(uint32_t sp, uint32_t pc)
{
    __set_MSP(sp);
    typedef void (*app_entry_t)(void);
    app_entry_t app_entry = (app_entry_t)(pc | 0x1u);
    __DSB();
    __ISB();
    app_entry();
}

const system_if_t system_stm32f1 = {
    .reset            = port_system_reset,
    .set_vtor         = port_system_set_vtor,
    .disable_irq      = port_system_disable_irq,
    .enable_irq       = port_system_enable_irq,
    .get_unique_id    = port_system_get_unique_id,
    .jump             = port_system_jump,
    .disable_all_irq  = port_system_disable_all_irq,
};
