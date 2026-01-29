  .section .text,
  .globl ctx_sw


# Context: 
# context: register a0
# new_context: register a1


ctx_sw:  
save_context:
  # We save the context of the current process
  sd ra, 8 * 0(a0)
  sd sp, 8 * 1(a0)
  sd gp, 8 * 2(a0)
  sd tp, 8 * 3(a0)
  sd s0, 8 * 4(a0)
  sd s1, 8 * 5(a0)
  sd s2, 8 * 6(a0)
  sd s3, 8 * 7(a0)
  sd s4, 8 * 8(a0)
  sd s5, 8 * 9(a0)
  sd s6, 8 * 10(a0)
  sd s7, 8 * 11(a0)
  sd s8, 8 * 12(a0)
  sd s9, 8 * 13(a0)
  sd s10,8 * 14(a0) 
  sd s11, 8 * 15(a0)

  csrr t0, mepc
  csrr t1, mstatus

  sd t0, 8 * 16(a0)
  sd t1, 8 * 17(a0)
restore_new_context:
  # We get the context associate to the new process
  ld ra, 8 * 0(a1)
  ld sp, 8 * 1(a1)
  ld gp, 8 * 2(a1)
  ld tp, 8 * 3(a1)
  ld s0, 8 * 4(a1)
  ld s1, 8 * 5(a1)
  ld s2, 8 * 6(a1)
  ld s3, 8 * 7(a1)
  ld s4, 8 * 8(a1)
  ld s5, 8 * 9(a1)
  ld s6, 8 * 10(a1)
  ld s7, 8 * 11(a1)
  ld s8, 8 * 12(a1)
  ld s9, 8 * 13(a1)
  ld s10,8 * 14(a1) 
  ld s11, 8 * 15(a1)
  ld t0, 8 * 16(a1)
  ld t1, 8 * 17(a1)


  csrw mepc, t0
  csrw mstatus, t1
  
  # return 
  ret 


