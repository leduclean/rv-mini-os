#include "process.h"
#include "scheduler.h"
#include "time.h"
#include <console.h>
#include <cpu.h>
#include <stdio.h>

// Process init
ptable_t proc_table = {.current_pid = 0};
process_t *actif;

extern void mon_traitant(void);
void proc1() {
  for (;;) {
    printf("[%s] pid = %i\n", mon_nom(), mon_pid());
    ordonnance();
  }
}

void proc2() {
  for (;;) {
    printf("[%s] pid = %i\n", mon_nom(), mon_pid());
    ordonnance();
  }
}

void kernel_start() {
  init_proc();
  init_ecran();
  creer_processus(proc1, "proc1");
  creer_processus(proc2, "marius");
  idle();
}
