/*
 * Trap entry and exit, shared by the user and the kernel paths.
 *
 * Convention: sscratch holds the top of the kernel stack of the current
 * process while it runs in user mode, and 0 while we are in the kernel.
 *
 * Offsets follow struct pt_regs in trap.h, keep both in sync.
 */

    .section .text
    .global trap_entry, trap_return
trap_entry:
    csrrw sp, sscratch, sp      // sp = kernel sp (from U) | 0 (from M)
    bnez  sp, _from_user

_from_kernel:
    csrrw sp, sscratch, sp      // take our sp back, sscratch returns to 0
    addi  sp, sp, -8 * 32
    sd    t0, 10 * 8(sp)        // free a working register
    addi  t0, sp, 8 * 32        // the interrupted sp
    sd    t0,  0 * 8(sp)
    j     _save_rest

_from_user:
    // Use the kernel sp to save the state
    addi  sp, sp, -8 * 32

    // Save the stack pointer of the kernel
    sd    t0, 10 * 8(sp)
    csrr  t0, sscratch
    sd    t0,  0 * 8(sp)
    
    // We are in the kernel from now on 
    csrw  sscratch, zero 

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
    
    // The stack now points 
    // to the top of the structure
    mv  a0, sp
    // trap_handler(pt_regs_t *)
    jal trap_handler

trap_return:
    ld   t0, 31 * 8(sp)
    csrw sstatus, t0

    // SPP mask
    li   t1, 0x100
    and  t1, t0, t1

    // SPP != 0 means we don't return to user mode
    bnez t1, _restore_rest

    // Back to user so sscratch is top of the kernel stack 
    // (without the context allocation)
    addi t1, sp, 8 * 32
    csrw sscratch, t1
_restore_rest:
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

    ld sp,  0 * 8(sp)           // sp last, it is the base of every load above

    sret
