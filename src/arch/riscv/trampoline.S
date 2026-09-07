#include  "asm_defs.h"

/*
 * Trap entry from user mode.
 * Offsets follow struct tframe_t in trap.h, keep both in sync.
 */
.section .trampoline, "ax", @progbits
.global trap_entry, trap_return

/* We start in supervisor mode but with
 * the page table of the user mode */
trap_entry:
    // User:
    //   sp       = USTACK
    //   sscratch = TRAPFRAME
    //   stvec    = TRAMPOLINE


    // sp = TRAPFRAME = tframe_t
    csrrw sp, sscratch, sp

    // Use the structure so save the state
    // Save t0 to use it as scratch
    sd  t0, 10 * 8(sp)

    // Save the user mode sp which is in sscratch
    csrr  t0, sscratch
    sd    t0,  0 * 8(sp) 

_save_rest:
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

    // t[7], t0 is already saved above
    sd t1, 11 * 8(sp)
    sd t2, 12 * 8(sp)
    sd t3, 13 * 8(sp)
    sd t4, 14 * 8(sp)
    sd t5, 15 * 8(sp)
    sd t6, 16 * 8(sp)

    // s[12]
    sd s0,  17 * 8(sp)
    sd s1,  18 * 8(sp)
    sd s2,  19 * 8(sp)
    sd s3,  20 * 8(sp)
    sd s4,  21 * 8(sp)
    sd s5,  22 * 8(sp)
    sd s6,  23 * 8(sp)
    sd s7,  24 * 8(sp)
    sd s8,  25 * 8(sp)
    sd s9,  26 * 8(sp)
    sd s10, 27 * 8(sp)
    sd s11, 28 * 8(sp)

    csrr t0, sepc
    sd   t0, 29 * 8(sp)
    csrr t0, scause
    sd   t0, 30 * 8(sp)
    csrr t0, sstatus
    sd   t0, 31 * 8(sp)
    csrr t0, satp
    sd   t0, 32 * 8(sp)
 
_kernel_config:
    // t0 = kstack
    ld t0, 33 * 8(sp)

    // t1 = kstap
    ld t1, 34 * 8(sp) 
    // sp = kstack
    mv sp, t0

    // Switch to kernel pages
    csrw  satp, t1 
    sfence.vma zero, zero 

    // Write zero in sscratch because 
    // we are now in kernel
    csrw  sscratch, zero 

    // Update the stvec to the kernelvec
    ld t0, 1f
    csrw stvec, t0
    // Load the handling function physical address
    ld t1, 2f
    j trap_handler

// Literral pooling because we are in the user 
// page table so the pc is virtual and too far from
// those function.
1:  .quad kernelvec
2:  .quad usertrap

trap_handler:
    // call usertrap()
    // The user trap will call returns the user pages satp in a0.
    jalr t1
// void trap_return(unsigned long usatp);
trap_return:    
    // When entering user mode, stvec should 
    // be the trampoline page again.
    li t0, TRAMPOLINE
    csrw stvec, t0

    // Switch back to user pages.
    csrw  satp, a0 
    sfence.vma zero, zero 

    li sp, TRAPFRAME

    ld   t0, 31 * 8(sp)
    csrw sstatus, t0
    ld   t0, 29 * 8(sp)
    csrw sepc, t0

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

    // s[12]
    ld s0,  17 * 8(sp)
    ld s1,  18 * 8(sp)
    ld s2,  19 * 8(sp)
    ld s3,  20 * 8(sp)
    ld s4,  21 * 8(sp)
    ld s5,  22 * 8(sp)
    ld s6,  23 * 8(sp)
    ld s7,  24 * 8(sp)
    ld s8,  25 * 8(sp)
    ld s9,  26 * 8(sp)
    ld s10, 27 * 8(sp)
    ld s11, 28 * 8(sp)

    // t[7] last, t0 and t1 were the working registers above
    ld t2, 12 * 8(sp)
    ld t3, 13 * 8(sp)
    ld t4, 14 * 8(sp)
    ld t5, 15 * 8(sp)
    ld t6, 16 * 8(sp)
    ld t1, 11 * 8(sp)
    ld t0, 10 * 8(sp)

    // scratch should be TRAPFRAME again
    csrw sscratch, sp
    ld sp,  0 * 8(sp)           // sp last, it is the base of every load above

    sret
