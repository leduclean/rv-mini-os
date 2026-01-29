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
  for (int i = 0; i < 2; i++) {
    printf("[temps = %u] processus %s pid = %i\n", nbr_secondes(), mon_nom(),
           mon_pid());
    dors(2);
  }
}
void proc2() {
  for (;;) {
    printf("[temps = %u] processus %s pid = %i\n", nbr_secondes(), mon_nom(),
           mon_pid());
    dors(3);
  }
}

void proc3() {
  for (;;) {
    printf("[temps = %u] processus %s pid = %i\n", nbr_secondes(), mon_nom(),
           mon_pid());
    dors(5);
  }
}

void kernel_start() {
  init_proc();
  init_ecran();
  enable_timer();
  init_traitant(mon_traitant);
  creer_processus(proc1, "bob1");
  creer_processus(proc2, "bob2");
  // creer_processus(proc3, "bob3");
  idle();
}
