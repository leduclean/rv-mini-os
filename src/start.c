#include "process.h"
#include "scheduler.h"
#include "time.h"
#include <console.h>
#include <cpu.h>
#include <stddef.h>
#include <stdio.h>
// Process init
ptable_t proc_table = {.next_pid = 0};
run_queue_t run_queue = {.head = 0, .tail = 0};
process_t *active;

extern void mon_traitant(void);

void proc1() {
  for (int i = 0; i < 2; i++) {
    printf("[temps = %u] processus %s pid = %i\n", nbr_secondes(), mon_nom(),
           mon_pid());
    dors(2);
  }
}

void proc3() {
  for (;;) {
    printf("[temps = %u] processus %s pid = %i\n", nbr_secondes(), mon_nom(),
           mon_pid());
    dors(1);
  }
}

void proc2() {
  for (int i = 0; i < 10; i++) {
    printf("[temps = %u] processus %s pid = %i\n", nbr_secondes(), mon_nom(),
           mon_pid());
    if (i == 1) {
      cree_processus(proc3, "bob3");
    }
    dors(3);
  }
}

void proc4() {}

void kernel_start() {
  init_proc();
  init_ecran();
  enable_timer();
  init_traitant(mon_traitant);
  cree_processus(proc1, "bob1");
  cree_processus(proc2, "bob2");
  idle();
}
