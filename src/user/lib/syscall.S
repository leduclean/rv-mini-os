# TODO: Since this is specific to riscv asm maybe it should resid on the arch/riscv/
.text
.global syscall

# long syscall(long n, long a, long b, long c, long d, long e, long f)
syscall:
    mv a7, a0
    mv a0, a1
    mv a1, a2
    mv a2, a3
    mv a3, a4
    mv a4, a5
    mv a5, a6
    ecall
    ret
    
