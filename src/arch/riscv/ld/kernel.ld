OUTPUT_ARCH( "riscv" )

PAGE_SIZE = 4K; /** = PAGE_SIZE in asm_defs.h (Sv39) */
DWORD_SIZE = 8; /** Align to a riscv dword */
KHEAP_SIZE = 2M;

MEMORY
{
    ram (wxa) : ORIGIN = 0x80000000, LENGTH = 128M
}

SECTIONS {
    _ram_start = ORIGIN(ram);

    . = 0x80000000;
    .text ALIGN(PAGE_SIZE) : {
        *(.text.init)

        /* All the user code should go to the user_text region */
        *(EXCLUDE_FILE(*user.dir/*) .text)
        *(EXCLUDE_FILE(*user.dir/*) .text.*)
        . = ALIGN(PAGE_SIZE);
    } > ram

    /* TODO: drop once user.elf is loaded. User code, mapped U in every process. */
    .user_text ALIGN(PAGE_SIZE) : {
        _user_start = .;
        *(.user_text.init)
        *(.user_text)
        *(.user_text.*)
        *user.dir/*(.text .text.*)
        *user.dir/*(.rodata .rodata.*)
        . = ALIGN(PAGE_SIZE);
        _user_end = .;
    } > ram

    /* Trampoline shared section*/
    .trampoline ALIGN(PAGE_SIZE) : {
        _trampoline_start = . ;
        KEEP(*(.trampoline))
        . = ALIGN(PAGE_SIZE);
        _trampoline_end = .;
    } > ram

    /* Data section */
    .data ALIGN(PAGE_SIZE) : {
        *(.sdata) *(.sdata.*)
        *(.fini)
        *(.anno)
        *(.rodata) *(.rodata.*)
        *(__ex_table)
        *(.data) *(.data.*)
        . = ALIGN(PAGE_SIZE);
    } > ram

    /* BSS section */
    .bss ALIGN(DWORD_SIZE) : {
        _bss_start = .;
        *(.sbss) *(.sbss.*)
        *(.bss) *(.bss.*)
        *(scommon) *(COMMON)
        . = ALIGN(DWORD_SIZE);
        _bss_end = .;
    } > ram

    /* Kernel heap (tinyalloc) */
    .kheap ALIGN(PAGE_SIZE) (NOLOAD) : {
        _kheap_start = .;
        . += KHEAP_SIZE;
        _kheap_end = .;
    } > ram

    _kernel_end = .;

    _ram_end = ORIGIN(ram) + LENGTH(ram);

    /* Page allocator symbols */
    _page_start = ALIGN(_kernel_end, PAGE_SIZE);
    _page_end   = _ram_end;

    ASSERT(_page_end > _page_start, "kernel image too big: no ram left for pages")
}

