/** This file define the locking premitive using riscv atomics **/
.section text
.globl amoswap
.globl amoswap_rl

/* unsigned long amoswap(unsigned int new, unsigned int *val) */
amoswap:
    amoswap.w.aq a0, a0, (a1)
    ret
/* unsigned long amoswap_rl(unsigned int *val) */
amoswap_rl:
    amoswap.w.rl zero, zero, (a0)
    ret

    

