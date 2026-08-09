/**
 * @file port_flash.c
 * @brief STM32F103 Flash interface
 *
 * Key differences between F103 and F407:
 *   1. Page erase (1KB/page) vs sector erase (16K/64K/128K)
 *   2. Must use half-word (16-bit) programming vs optional 1/2/4/8 bytes
 *   3. No concept of "sector", only "page"
 *   4. CPU instruction fetch stalls during Flash erase/program (no cache); interrupts must be disabled
 *
 * About CR mode bit clearing: PG/PER/MER and other mode bits in F103 FLASH->CR
 * do not auto-clear on operation completion and must be cleared by software.
 * If they remain set (especially PER), the PG bit of the next operation coexists
 * with the leftover PER and the hardware silently ignores writes.
 * Therefore, on entry to erase/program we clear all mode bits defensively.
 */

#include "../hal_if_defines.h"
#include "port.h"
#include "main.h"
#include <string.h>

static int port_flash_init(void) { return 0; }

static int port_flash_unlock(void)
{
    return (HAL_FLASH_Unlock() == HAL_OK) ? 0 : -1;
}

static int port_flash_lock(void)
{
    return (HAL_FLASH_Lock() == HAL_OK) ? 0 : -1;
}

static int port_flash_erase(uint32_t addr, uint32_t size)
{
    uint32_t page_addr = addr & ~0x3FFU;   /* Align to 1KB page */
    uint32_t end_addr  = addr + size;

    __disable_irq();

    /* Defensive unlock */
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
        if (FLASH->CR & FLASH_CR_LOCK) {
            __enable_irq();
            return -1;
        }
    }

    /* Clear leftover mode bits and error flags */
    FLASH->CR &= ~(FLASH_CR_PG | FLASH_CR_PER | FLASH_CR_MER
                   | FLASH_CR_OPTPG | FLASH_CR_OPTER);
    FLASH->SR = FLASH_SR_PGERR | FLASH_SR_WRPRTERR | FLASH_SR_EOP;

    while (page_addr < end_addr) {
        while (FLASH->SR & FLASH_SR_BSY) {}

        FLASH->CR |= FLASH_CR_PER;
        FLASH->AR  = page_addr;
        FLASH->CR |= FLASH_CR_STRT;

        while (FLASH->SR & FLASH_SR_BSY) {}

        FLASH->CR &= ~FLASH_CR_PER;

        if (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) {
            __enable_irq();
            return -1;
        }
        page_addr += 0x400U;
    }

    __enable_irq();
    return 0;
}

static int port_flash_program(uint32_t addr, const uint8_t *data, uint32_t size)
{
    uint32_t i = 0;

    __disable_irq();

    /* Defensive unlock */
    if (FLASH->CR & FLASH_CR_LOCK) {
        FLASH->KEYR = 0x45670123;
        FLASH->KEYR = 0xCDEF89AB;
        if (FLASH->CR & FLASH_CR_LOCK) {
            __enable_irq();
            return -1;
        }
    }

    /* Clear leftover mode bits and error flags.
     * Note: if a previous erase left PER=1, the PG=1 below produces PG|PER
     * coexistence, and the F103 flash controller silently ignores all writes:
     * no PGERR, no WRPRTERR, the function appears "successful" but not a single
     * flash byte changes. This is a gotcha we have already hit. */
    FLASH->CR &= ~(FLASH_CR_PG | FLASH_CR_PER | FLASH_CR_MER
                   | FLASH_CR_OPTPG | FLASH_CR_OPTER);
    FLASH->SR = FLASH_SR_PGERR | FLASH_SR_WRPRTERR | FLASH_SR_EOP;

    while (i + 1 < size) {
        while (FLASH->SR & FLASH_SR_BSY) {}
        FLASH->CR |= FLASH_CR_PG;
        *(__IO uint16_t *)(addr + i) = (uint16_t)(data[i] | (data[i + 1] << 8));
        while (FLASH->SR & FLASH_SR_BSY) {}
        FLASH->CR &= ~FLASH_CR_PG;

        if (FLASH->SR & (FLASH_SR_PGERR | FLASH_SR_WRPRTERR)) {
            __enable_irq();
            return -1;
        }
        i += 2;
    }

    /* Trailing odd byte: pad high byte with 0xFF */
    if (i < size) {
        while (FLASH->SR & FLASH_SR_BSY) {}
        FLASH->CR |= FLASH_CR_PG;
        *(__IO uint16_t *)(addr + i) = (uint16_t)(data[i] | 0xFF00);
        while (FLASH->SR & FLASH_SR_BSY) {}
        FLASH->CR &= ~FLASH_CR_PG;
    }

    __enable_irq();
    return 0;
}

static int port_flash_read(uint32_t addr, uint8_t *data, uint32_t size)
{
    memcpy(data, (const void *)addr, size);
    return 0;
}

const flash_if_t flash_stm32f1 = {
    .init               = port_flash_init,
    .unlock             = port_flash_unlock,
    .lock               = port_flash_lock,
    .erase              = port_flash_erase,
    .program            = port_flash_program,
    .read               = port_flash_read,
    .base_addr          = 0x08000000,
    .total_size         = 64 * 1024,
    .sector_size        = FLASH_PAGE_SIZE,
    .program_granularity = 2,   /* F103 requires half-word programming */
};
