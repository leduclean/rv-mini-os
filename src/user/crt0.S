    .section .text.init,"ax",@progbits
    .globl _start

_start:
    la t0, _bss_start
    la t1, _bss_end 

# Raz bss 
loop: 
    bgeu t0, t1, end_loop
    sd zero, 0(t0)
    addi t0, t0, 8
    j loop
end_loop:
    lw a0, 0(sp)
    addi a1, sp, 16
    call main
    call exit_group
    
