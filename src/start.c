#include "console.h"
#include "cpu.h"
#include "process.h"
#include "scheduler.h"
#include "time.h"

#include "eval_aliases.h"

#include <stddef.h>
#include <stdio.h>

extern void mon_traitant(void);

void proc1() {
  for (int i = 0; i < 2; i++) {
    printf("[temps = %u] processus %s pid = %i\n", nbr_secondes(), mon_nom(),
           mon_pid());
    dors(2);
    ordonnance();
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
