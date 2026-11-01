    #include <abi.h>
    .equ STACK_SIZE, 4096
    .section .text.init,"ax",@progbits
    .globl entry

entry:
    /* Sp points to the top of the allocated .bss stack memory. */
    la sp, boot_stack_top

    # RAZ of bss.
    la t0, _bss_start
    la t1, _bss_end
1:  sd zero, 0(t0)
    addi t0, t0, 8
    bltu t0, t1, 1b

    // M-mode boot
    j start

    .bss
    .balign STACK_ALIGN
boot_stack:
    .skip STACK_SIZE
boot_stack_top:
