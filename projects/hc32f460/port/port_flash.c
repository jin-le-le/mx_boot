/**
 * @file port_flash.c
 * @brief HC32F460 Flash interface (EFM peripheral)
 *
 * Strategy:
 *   - port_flash_init sets EFM_FWMC_Cmd(ENABLE) + BUS_HOLD once
 *   - port_flash_unlock / lock are no-ops (DDL manages mode switching internally)
 *   - Uses EFM_Program (PGM_SINGLE) instead of EFM_SequenceProgram (more reliable)
 */

#include "../hal_if_defines.h"
#include "port.h"
#include <string.h>

static int port_flash_init(void)
{
    /* Wait for Flash ready */
    while (SET != EFM_GetStatus(EFM_FLAG_RDY)) {
    }
    /* Enable FWMC (Flash Write Mode Control); DDL API requires this to write the FWMC register */
    EFM_FWMC_Cmd(ENABLE);
    /* Hold bus during erase/program (hardware automatically stalls the CPU) */
    EFM_SetBusStatus(EFM_BUS_HOLD);
    return 0;
}

static int port_flash_unlock(void)
{
    /* DDL manages mode switching internally; here we only need to ensure FWMC is unlocked */
    while (SET != EFM_GetStatus(EFM_FLAG_RDY)) {
    }
    return 0;
}

static int port_flash_lock(void)
{
    /* Wait for Flash ready (ensure operation completes) */
    while (SET != EFM_GetStatus(EFM_FLAG_RDY)) {
    }
    return 0;
}

static int port_flash_erase(uint32_t addr, uint32_t size)
{
    uint32_t sector_addr = addr & ~((uint32_t)HC32F4_SECTOR_SIZE - 1U);
    uint32_t end_addr    = addr + size;

    while (sector_addr < end_addr) {
        int32_t ret = EFM_SectorErase(sector_addr);
        if (ret != LL_OK) {
            return -1;
        }
        sector_addr += HC32F4_SECTOR_SIZE;
    }
    return 0;
}

static int port_flash_program(uint32_t addr, const uint8_t *data, uint32_t size)
{
    /* Use EFM_Program (PGM_SINGLE mode) instead of EFM_SequenceProgram
     * PGM_SINGLE is more reliable: each word is written and awaited independently */
    int32_t ret = EFM_Program(addr, data, size);
    if (ret != LL_OK) {
        return -1;
    }
    return 0;
}

static int port_flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    memcpy(data, (const void *)addr, size);
    return 0;
}

const flash_if_t flash_hc32f4 = {
    .init                = port_flash_init,
    .unlock              = port_flash_unlock,
    .lock                = port_flash_lock,
    .erase               = port_flash_erase,
    .program             = port_flash_program,
    .read                = port_flash_read,
    .base_addr           = 0x00000000U,
    .total_size          = 512 * 1024,
    .sector_size         = HC32F4_SECTOR_SIZE,
    .program_granularity = 4,
};
