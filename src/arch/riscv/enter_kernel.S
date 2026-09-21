#include <asm/csr.h>
.text
.globl enter_kernel, delegate_traps

// void enter_kernel(void (*entry)())
enter_kernel: 
    // Set mepc to the entry point
    csrw mepc, a0

    // Previous privilege to supervisor mode.
    csrr t0, mstatus
   
    li t1, MSTATUS_MPP_MASK
    not t1, t1                
    and t0, t0, t1

    li t1, PRV_S
    slli t1, t1, MSTATUS_MPP_SHIFT


    or t0, t0, t1  
    csrw mstatus, t0

    mret


delegate_traps:
    // Delegate all exceptions to the supervisor mode
    li t0, -1 
    csrw medeleg, t0

    // Delegate supervisor interrupts directly to the supervisor mode
    li t0, 1
    slli t0, t0, S_IRQ_EXT

    li t1, 1
    slli t1, t1, S_IRQ_TMR

    or t0, t1, t0

    csrs mideleg, t0
    ret



