/*
** STM32F407ZG Bootloader Linker Script
** 1024KB Flash, 192KB RAM
**
** Memory layout:
** - Bootloader: 0x08000000 - 0x0800BFFF (48KB)
** - App:        0x08010000 - 0x080FFFFF (1024KB - 48KB - 64KB = 896KB)
** - Header:     0x0800C000 - 0x080FFFFF (reserved for magic header and future use)
*/

ENTRY(Reset_Handler)

/* Flash and RAM configuration */
FLASH_BASE = 0x08000000;
FLASH_SIZE = 1024K;
RAM_BASE   = 0x20000000;
RAM_SIZE   = 192K;  /* STM32F407ZGT6 has 192KB SRAM */

/* Bootloader region */
BOOTLOADER_SIZE = 48K;
BOOTLOADER_END   = FLASH_BASE + BOOTLOADER_SIZE;

/* Application region */
APP_BASE   = BOOTLOADER_END;              /* 0x08010000 */
APP_SIZE   = FLASH_SIZE - BOOTLOADER_SIZE - 64K;  /* 896KB */

/* Magic header at sector 11 boundary */
MAGIC_HEADER_ADDR = 0x0800C000;

MEMORY
{
    FLASH (rx)  : ORIGIN = FLASH_BASE, LENGTH = FLASH_SIZE
    RAM   (rwx) : ORIGIN = RAM_BASE,   LENGTH = RAM_SIZE
}

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        _boot_start = .;
        _start_text = .;
        KEEP(*(.isr_vector))
        *(.vector_table)
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text*)
        *(.rodata*)
        *(.ARM.exidx*)
        *(.rodata)
        . = ALIGN(4);
        _end_text = .;
    } > FLASH

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    _sidata = LOADADDR(.data);

    /DISCARD/ :
    {
        libc.a (*)
        libm.a (*)
        libgcc.a (*)
    }

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
    } > RAM

    .heap :
    {
        . = ALIGN(8);
        _heap_start = .;
        . = . + 2K;
        . = ALIGN(8);
        _heap_end = .;
    } > RAM

    .stack :
    {
        . = ALIGN(8);
        . = . + 2K;
        . = ALIGN(8);
        _estack = .;
    } > RAM

    _Min_Stack_Size = 0x1200; /* 4.5KB for MSP stack */

    _end = .;

    /* Total bootloader footprint in flash = end of .data LMA.
     * (.data AT > FLASH: its initial values live in flash, so include them.) */
    _boot_flash_used = LOADADDR(.data) + SIZEOF(.data) - FLASH_BASE;
}

/*===================================================================
 * Link-time assertion: actual bootloader size must not exceed BOOTLOADER_SIZE.
 *
 * Otherwise it spills into the header/app region and gets erased by its own
 * ERASE command during upgrade. Symptom: after a successful erase call, any
 * subsequent s_plat->xxx->yyy() dereference HardFaults.
 * If this assertion fires, raise BOOTLOADER_SIZE (keep sector alignment) or
 * shrink the code (disable BL_LOG_ENABLED, switch CRC32 to bitwise, etc.).
 *===================================================================*/
ASSERT(_boot_flash_used <= BOOTLOADER_SIZE, "ERROR: Bootloader exceeds BOOTLOADER_SIZE, will self-erase on upgrade")
