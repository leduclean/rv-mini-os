OUTPUT_ARCH( "riscv" )

MEMORY
{
    ram (wxa!ri) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS {
    _ram_start = ORIGIN(ram);

    . = 0x80000000;
    .text : {
        *(.text.init) *(.text) *(.text.*)
    } > ram

    . = ALIGN(4096);
    _etext = .;

    /* Section Utilisateur */
    .user_text ALIGN(4096) : {
        _user_start = . ;
        *(.user_text)
        *(.user_text.*)
    } > ram
    . = ALIGN(4096);
    _user_end = .;

    /* Section Trampoline */
    .trampoline ALIGN(4096) : {
        _trampoline_start = . ;
        KEEP(*(.trampoline))
    } > ram
    . = ALIGN(4096);
    _trampoline_end = .;

    /* Section Données */
    .data ALIGN(4096) : {
        *(.sdata) *(.sdata.*)
        *(.fini)
        *(.anno)
        *(.rodata) *(.rodata.*)
        *(__ex_table)
        *(.data) *(.data.*)
        _edata = .;
    } > ram

    /* Section BSS */
    .bss : {
        . = ALIGN(8);
        __bss_start = .;
        *(.sbss) *(.sbss.*)
        *(.bss) *(.bss.*)
        *(scommon) *(COMMON)
        . = ALIGN(8);
        __bss_end = .;
        PROVIDE(_heap_start = __bss_end + 8);
        PROVIDE(_heap_end = ORIGIN(ram) + LENGTH(ram));
    } > ram
    _end = .;
}
