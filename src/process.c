#include "process.h"
#include "cpu.h"
#include <stdint.h>
#include <stdio.h>

#define PROC_TABLE_SIZE 2

process_t proc_table[PROC_TABLE_SIZE];

extern void ctx_sw(uintptr_t old_context_addr, uintptr_t new_context_addr);

#define RA_INDEX 0
#define SP_INDEX 1

process_t idle_proc = {
    .pid = 0,
    .name = "idle",
    .state = ELECTED,
};

process_t proc1_proc = {
    .pid = 1,
    .name = "proc1",
    .state = ACTIVABLE,
    .ctx = {0},
    .stack = {0},
};

void init_proc() {
  proc1_proc.ctx[SP_INDEX] = (uint64_t)&proc1_proc.stack[STACK_SIZE - 1];
  proc1_proc.ctx[RA_INDEX] = (uintptr_t)proc1;
  proc_table[0] = idle_proc;
  proc_table[1] = proc1_proc;
}

void idle() {
  printf("[idle] je tente de passer la main a proc1...\n");
  ctx_sw((uintptr_t)&idle_proc.ctx, (uintptr_t)&proc1_proc.ctx);
}

void proc1() {
  printf("[proc1] idle m'a donne la main\n");
  printf("[proc1] j'arrete le systeme\n");
  hlt();
}
// void idle() {
//   for (int i = 0; i < 3; i++) {
//     printf("[idle] je tente de passer la main a proc1...\n");
//     ctx_sw((uintptr_t)&idle_proc.save, (uintptr_t)&proc1_proc.save);
//     printf("[idle] proc1 m'a redonne la main\n");
//   }
//   printf("[idle] je bloque le systeme\n");
//   hlt();
// }
//
// void proc1() {
//   for (;;) {
//     printf("[proc1] idle m'a donne la main\n");
//     printf("[proc1] je tente de lui la redonner...\n");
//     ctx_sw((uintptr_t)&proc1_proc.save, (uintptr_t)&idle_proc.save);
//   }
// }
