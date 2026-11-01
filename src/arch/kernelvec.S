/*
 * Trap entry path for a kernel trap.
 * This is a simplified path than trampoline.S.
 * because we keep only one page table for the kernel 
 * which is kroot.
 */
.text
.globl kernelvec
kernelvec:
    // Kernel:
    //   sp       = kstack 
    //   sscratch = 0
    //   stvec    = kernelvec

    addi sp, sp, -8 * 17
    // Save caller-saved registers
    sd ra,  1 * 8(sp)
    // a[8]
    sd a0,  2 * 8(sp)
    sd a1,  3 * 8(sp)
    sd a2,  4 * 8(sp)
    sd a3,  5 * 8(sp)
    sd a4,  6 * 8(sp)
    sd a5,  7 * 8(sp)
    sd a6,  8 * 8(sp)
    sd a7,  9 * 8(sp)

    // t[7]
    sd t0, 10 * 8(sp)
    sd t1, 11 * 8(sp)
    sd t2, 12 * 8(sp)
    sd t3, 13 * 8(sp)
    sd t4, 14 * 8(sp)
    sd t5, 15 * 8(sp)
    sd t6, 16 * 8(sp)
   
    // void kerneltrap();
    call kerneltrap

    // Restore caller-saved registers
    ld ra,  1 * 8(sp)
    // a[8]
    ld a0,  2 * 8(sp)
    ld a1,  3 * 8(sp)
    ld a2,  4 * 8(sp)
    ld a3,  5 * 8(sp)
    ld a4,  6 * 8(sp)
    ld a5,  7 * 8(sp)
    ld a6,  8 * 8(sp)
    ld a7,  9 * 8(sp)

    // t[7]
    ld t0, 10 * 8(sp)
    ld t1, 11 * 8(sp)
    ld t2, 12 * 8(sp)
    ld t3, 13 * 8(sp)
    ld t4, 14 * 8(sp)
    ld t5, 15 * 8(sp)
    ld t6, 16 * 8(sp)

    addi sp, sp, 8 * 17
    sret
