OUTPUT_ARCH("riscv")

MEMORY {
    ram(wxa) :  ORIGIN = 0x10000, LENGTH = 128M
}

PAGE_SIZE = 4K; /** = PAGE_SIZE in asm_defs.h (Sv39) */
DWORD_SIZE = 8; /** Align to a riscv dword */

/* Programm headers */
PHDRS {
    text   PT_LOAD FLAGS(5); /* PH_R (4) + PH_X (1) = 5 */
    rodata PT_LOAD FLAGS(4); /* PH_R (4) = 4 */
    data   PT_LOAD FLAGS(6); /* PH_R (4) + PH_W (2) = 6 */
}

SECTIONS {
    . = 0x10000;

    .text ALIGN(PAGE_SIZE) : {
        *(.text.init)
        *(.text .text.*)
    } > ram : text

    .rodata ALIGN(PAGE_SIZE) : {
        *(.rodata .rodata.* .srodata .srodata.*)
    } > ram : rodata

    .data ALIGN(PAGE_SIZE) : {
        *(.data .data.* .sdata .sdata.*)
    } > ram : data

    .bss ALIGN(DWORD_SIZE) : {
        _bss_start = .;
        *(.bss .bss.* .sbss .sbss.*)
        . = ALIGN(DWORD_SIZE);
        _bss_end = .;
    } > ram : data
}

