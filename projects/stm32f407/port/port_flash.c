/**
 * @file port_flash.c
 * @brief Flash interface implementation with write protection
 */

#define LOG_TAG "flash"
#define LOG_LVL ELOG_LVL_INFO
#include "elog.h"

#include "../hal_if_defines.h"
#include "port.h"
#include "main.h"
#include <string.h>

static const uint32_t s_sectors[] = {
    FLASH_SECTOR_0, FLASH_SECTOR_1, FLASH_SECTOR_2, FLASH_SECTOR_3,
    FLASH_SECTOR_4, FLASH_SECTOR_5, FLASH_SECTOR_6, FLASH_SECTOR_7,
    FLASH_SECTOR_8, FLASH_SECTOR_9, FLASH_SECTOR_10, FLASH_SECTOR_11,
};

static const uint32_t s_sector_sizes[] = {
    STM32F4_SECTOR_SIZE_0, STM32F4_SECTOR_SIZE_1, STM32F4_SECTOR_SIZE_2, STM32F4_SECTOR_SIZE_3,
    STM32F4_SECTOR_SIZE_4, STM32F4_SECTOR_SIZE_5, STM32F4_SECTOR_SIZE_6, STM32F4_SECTOR_SIZE_7,
    STM32F4_SECTOR_SIZE_8, STM32F4_SECTOR_SIZE_9, STM32F4_SECTOR_SIZE_10, STM32F4_SECTOR_SIZE_11,
};

#define NUM_SECTORS (sizeof(s_sectors) / sizeof(s_sectors[0]))

/**
 * @brief Configure Flash write protection
 * @note This should be called once during bootloader init
 */
static void port_flash_protect(void)
{
#if BL_FLASH_PROTECTION_ENABLED
    /* Flash write protection is configured via option bytes */
    /* Note: RDP level 1 is standard protection, level 2 is irreversible */

    /* Bootloader sector protection - protect sectors 0-2 (bootloader area) */
    /* This prevents accidental erasure of bootloader */

    /* For production, consider setting RDP level via STM32CubeProgrammer */
    /* This function provides runtime protection configuration */

    FLASH_OBProgramInitTypeDef OBInit;
    HAL_FLASH_OB_Unlock();
    HAL_FLASHEx_OBGetConfig(&OBInit);

    /* Set RDP level */
    OBInit.OptionType = OPTIONBYTE_RDP;
    OBInit.RDPLevel = BL_RDP_LEVEL;
    HAL_FLASHEx_OBProgram(&OBInit);

    HAL_FLASH_OB_Lock();
#endif
}

static int port_flash_init(void)
{
    /* Configure flash protection on init */
    port_flash_protect();
    return 0;
}

static void port_flash_deinit(void)
{
    HAL_FLASH_Lock();
}

static int port_flash_unlock(void)
{
    HAL_FLASH_Unlock();
    return 0;
}

static int port_flash_lock(void)
{
    HAL_FLASH_Lock();
    return 0;
}

static int port_flash_erase(uint32_t addr, uint32_t size)
{
    uint32_t flash_base = BL_FLASH_BASE;
    uint32_t end_addr = addr + size;
    uint32_t primask;

    log_i("port_flash_erase: addr=0x%08lX size=%lu", addr, (unsigned long)size);
    log_i("port_flash_erase: end_addr=0x%08lX", end_addr);

    if (addr < flash_base || end_addr > flash_base + BL_FLASH_TOTAL) {
        log_e("port_flash_erase: address out of range");
        return -1;
    }

    /* Disable interrupts during flash erase to prevent data corruption */
    primask = __get_PRIMASK();
    __disable_irq();

    /* Disable flash data cache */
    __HAL_FLASH_DATA_CACHE_DISABLE();
    __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();

    HAL_FLASH_Unlock();

    /* Clear any existing error flags */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    /* Check SR after clear */
    if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR)) {
        log_w("port_flash_erase: error flags still set after clear!");
    }

    /* Erase all sectors that overlap with [addr, end_addr) */
    uint32_t sector_start = flash_base;
    for (uint32_t i = 0; i < NUM_SECTORS; i++) {
        uint32_t sector_end = sector_start + s_sector_sizes[i];
        log_i("port_flash_erase: checking sector %lu [0x%08lX, 0x%08lX), addr=0x%08lX end_addr=0x%08lX",
              (unsigned long)i, (unsigned long)sector_start, (unsigned long)sector_end, addr, end_addr);
        /* Check if sector overlaps with erase range: sector_start < end_addr && sector_end > addr */
        if (sector_start < end_addr && sector_end > addr) {
            log_i("port_flash_erase: ERASING sector %lu s_sectors[%lu]=%lu", (unsigned long)i, (unsigned long)i, (unsigned long)s_sectors[i]);
            FLASH_Erase_Sector(s_sectors[i], FLASH_VOLTAGE_RANGE_3);
            /* Wait for erase to complete */
            uint32_t wait_count = 0;
            while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY)) {
                wait_count++;
                if (wait_count > 0x100000) {
                    log_e("port_flash_erase: timeout waiting for BSY clear");
                    HAL_FLASH_Lock();
                    __HAL_FLASH_DATA_CACHE_ENABLE();
                    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
                    __set_PRIMASK(primask);
                    return -1;
                }
            }
            log_i("port_flash_erase: sector %lu erase complete, wait_count=%lu", (unsigned long)i, (unsigned long)wait_count);
            /* Verify no errors occurred */
            if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR)) {
                log_e("port_flash_erase: flash error flag set for sector %lu", (unsigned long)i);
                HAL_FLASH_Lock();
                __HAL_FLASH_DATA_CACHE_ENABLE();
                __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
                __set_PRIMASK(primask);
                return -1;
            }
        }
        sector_start = sector_end;
    }

    /* Wait for any last operation to complete */
    while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY)) {
    }

    /* Verify EOP is set */
    if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_EOP)) {
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP);
        log_i("port_flash_erase: EOP flag set");
    }

    HAL_FLASH_Lock();

    /* Invalidate and re-enable flash cache */
    __HAL_FLASH_DATA_CACHE_RESET();
    __HAL_FLASH_INSTRUCTION_CACHE_RESET();
    __HAL_FLASH_DATA_CACHE_ENABLE();
    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();

    __set_PRIMASK(primask);
    return 0;
}

static int port_flash_program(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t primask;

    /* Disable interrupts during flash programming to prevent data corruption */
    primask = __get_PRIMASK();
    __disable_irq();

    /* Disable flash data cache before programming */
    __HAL_FLASH_DATA_CACHE_DISABLE();
    __HAL_FLASH_INSTRUCTION_CACHE_DISABLE();

    HAL_FLASH_Unlock();

    /* Clear any existing error flags */
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR | FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    for (uint32_t i = 0; i < size; i += 4) {
        uint32_t word;
        uint32_t remain = size - i;
        uint32_t chunk = (remain < 4) ? remain : 4;
        memcpy(&word, data + i, chunk);
        /* Zero out remaining bytes when chunk < 4 */
        if (chunk < 4) {
            memset(((uint8_t *)&word) + chunk, 0, 4 - chunk);
        }
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr + i, word) != HAL_OK) {
            log_e("program: HAL_FLASH_Program failed at offset %lu", (unsigned long)i);
            HAL_FLASH_Lock();
            __HAL_FLASH_DATA_CACHE_ENABLE();
            __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();
            __set_PRIMASK(primask);
            return -1;
        }
        /* Wait for programming to complete */
        while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY)) {
        }
    }

    /* Wait for any last operation to complete */
    while (__HAL_FLASH_GET_FLAG(FLASH_FLAG_BSY)) {
    }

    HAL_FLASH_Lock();

    /* Invalidate and re-enable flash cache */
    __HAL_FLASH_DATA_CACHE_RESET();
    __HAL_FLASH_INSTRUCTION_CACHE_RESET();
    __HAL_FLASH_DATA_CACHE_ENABLE();
    __HAL_FLASH_INSTRUCTION_CACHE_ENABLE();

    __set_PRIMASK(primask);
    return 0;
}

static int port_flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    memcpy(data, (void *)addr, size);
    return 0;
}

const flash_if_t flash_stm32f4 = {
    .init               = port_flash_init,
    .deinit             = port_flash_deinit,
    .unlock             = port_flash_unlock,
    .lock               = port_flash_lock,
    .erase              = port_flash_erase,
    .program            = port_flash_program,
    .read               = port_flash_read,
    .base_addr          = BL_FLASH_BASE,
    .total_size         = BL_FLASH_TOTAL,
    .sector_size        = STM32F4_SECTOR_SIZE_0,
    .program_granularity = 4,
};
