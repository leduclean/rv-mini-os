  .section .text,
  .globl ctx_sw


# Context:
# context: register a0
# new_context: register a1
#
# Offsets follow struct ctx in kernel/process.h, keep both in sync.
# gp is a link-time constant and tp is per-hart, neither belongs here.


ctx_sw:
// Save current context
save_context:
  sd ra, 8 * 0(a0)
  sd sp, 8 * 1(a0)
  sd s0, 8 * 2(a0)
  sd s1, 8 * 3(a0)
  sd s2, 8 * 4(a0)
  sd s3, 8 * 5(a0)
  sd s4, 8 * 6(a0)
  sd s5, 8 * 7(a0)
  sd s6, 8 * 8(a0)
  sd s7, 8 * 9(a0)
  sd s8, 8 * 10(a0)
  sd s9, 8 * 11(a0)
  sd s10,8 * 12(a0)
  sd s11,8 * 13(a0)

// Load new context
restore_new_context:
  ld ra, 8 * 0(a1)
  ld sp, 8 * 1(a1)
  ld s0, 8 * 2(a1)
  ld s1, 8 * 3(a1)
  ld s2, 8 * 4(a1)
  ld s3, 8 * 5(a1)
  ld s4, 8 * 6(a1)
  ld s5, 8 * 7(a1)
  ld s6, 8 * 8(a1)
  ld s7, 8 * 9(a1)
  ld s8, 8 * 10(a1)
  ld s9, 8 * 11(a1)
  ld s10,8 * 12(a1)
  ld s11,8 * 13(a1)

  # return
  ret
