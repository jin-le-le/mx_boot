/*
** STM32F103C8T6 Bootloader Linker Script
** 64KB Flash, 20KB RAM
**
** Memory layout:
** - Bootloader: 0x08000000 - 0x08005FFF (24KB)
** - Header:     0x08006000 - 0x080063FF (1KB page)
** - App:        0x08006400 - 0x0800FFFF (40KB)
**
** NOTE: the bootloader compiles to ~21.4KB (micro-ecc + HAL + elog).
**       Allocating only 20KB caused it to spill into the app region and get
**       erased by its own ERASE command. We now reserve 24KB and add an
**       ASSERT at the end to enforce the limit at link time.
*/

ENTRY(Reset_Handler)

FLASH_BASE = 0x08000000;
FLASH_SIZE = 64K;
RAM_BASE   = 0x20000000;
RAM_SIZE   = 20K;

BOOTLOADER_SIZE = 24K;
BOOTLOADER_END   = FLASH_BASE + BOOTLOADER_SIZE;

APP_BASE   = 0x08006400;
APP_SIZE   = FLASH_SIZE - BOOTLOADER_SIZE - 1K;

MAGIC_HEADER_ADDR = 0x08006000;

MEMORY
{
    FLASH (rx)  : ORIGIN = FLASH_BASE, LENGTH = FLASH_SIZE
    RAM   (rwx) : ORIGIN = RAM_BASE,   LENGTH = RAM_SIZE
}

_Min_Heap_Size  = 0x200;
_Min_Stack_Size = 0x400;

/* Symbols required by the startup file and _sbrk */
_estack = ORIGIN(RAM) + LENGTH(RAM);    /* Stack top = end of RAM = 0x20005000 */

SECTIONS
{
    .isr_vector :
    {
        . = ALIGN(4);
        _boot_start = .;
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } > FLASH

    .text :
    {
        . = ALIGN(4);
        *(.text)
        *(.text*)
        *(.rodata)
        *(.rodata*)
        *(.glue_7)
        *(.glue_7t)
        *(.eh_frame)
        KEEP(*(.init))
        KEEP(*(.fini))
        . = ALIGN(4);
        _etext = .;
    } > FLASH

    .ARM.extab :
    {
        *(.ARM.extab* .gnu.linkonce.armextab.*)
    } > FLASH

    .ARM :
    {
        __exidx_start = .;
        *(.ARM.exidx*)
        __exidx_end = .;
    } > FLASH

    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH

    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH

    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH

    _sidata = LOADADDR(.data);

    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } > RAM AT > FLASH

    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } > RAM

    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        _end = .;
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } > RAM

    /DISCARD/ :
    {
        libc.a (*)
        libm.a (*)
        libgcc.a (*)
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }

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
 * If this assertion fires, raise BOOTLOADER_SIZE (keep 1KB page alignment) or
 * shrink the code (disable BL_LOG_ENABLED, switch CRC32 to bitwise, etc.).
 *===================================================================*/
ASSERT(_boot_flash_used <= BOOTLOADER_SIZE, "ERROR: Bootloader exceeds BOOTLOADER_SIZE, will self-erase on upgrade")
