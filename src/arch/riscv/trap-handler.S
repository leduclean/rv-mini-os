.section .text
.globl trap_vector

trap_vector: 
  # stack allocation
  # register save 
  addi  sp, sp, -16 * 8
  sd    ra,  0*8(sp)
  sd    a0,  1*8(sp)
  sd    a1,  2*8(sp)
  sd    a2,  3*8(sp)
  sd    a3,  4*8(sp)
  sd    a4,  5*8(sp)
  sd    a5,  6*8(sp)
  sd    a6,  7*8(sp)
  sd    a7,  8*8(sp)
  sd    t0,  9*8(sp)
  sd    t1, 10*8(sp)
  sd    t2, 11*8(sp)
  sd    t3, 12*8(sp)
  sd    t4, 13*8(sp)
  sd    t5, 14*8(sp)
  sd    t6, 15*8(sp)

  # set the arguments of the handler
  csrr    a0, mcause
  csrr    a1, mip
  csrr    a2, mie

  # call the handler 
  jal trap_handler


  ld    ra,  0*8(sp)
  ld    a0,  1*8(sp)
  ld    a1,  2*8(sp)
  ld    a2,  3*8(sp)
  ld    a3,  4*8(sp)
  ld    a4,  5*8(sp)
  ld    a5,  6*8(sp)
  ld    a6,  7*8(sp)
  ld    a7,  8*8(sp)
  ld    t0,  9*8(sp)
  ld    t1, 10*8(sp)
  ld    t2, 11*8(sp)
  ld    t3, 12*8(sp)
  ld    t4, 13*8(sp)
  ld    t5, 14*8(sp)
  ld    t6, 15*8(sp)
  
  # stack reallocation 
  addi sp, sp, 16 * 8
  mret
