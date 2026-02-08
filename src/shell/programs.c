#include "shell/programs.h"
#include "shell/cmd_registry.h"
#include "kernel/process/process.h"
#include "kernel/sched/scheduler.h"
#include "kernel/time/time.h"
#include "lib/stddef.h"
#include "lib/stdio.h"

#define MAX_CMDS 64

static void proc1() {
  for (int i = 0; i < 2; i++) {
    printf("[temps = %u] processus %s pid = %i\n", seconds(), get_active_name(),
           get_active_pid());
    scheduler_sleep(2);
  }
}

static void proc3() {
  for (;;) {
    printf("[temps = %u] processus %s pid = %i\n", seconds(), get_active_name(),
           get_active_pid());
    scheduler_sleep(1);
  }
}

static void proc2() {
  for (int i = 0; i < 10; i++) {
    printf("[temps = %u] processus %s pid = %i\n", seconds(), get_active_name(),
           get_active_pid());
    if (i == 1) {
      spawn_process(proc3, "bob3", HIGH);
    }
    scheduler_sleep(3);
  }
}

/** Register the programs **/
void init_programs() {
  register_prog("proc1", proc1, HIGH);
  register_prog("proc2", proc2, NORMAL);
  register_prog("proc3", proc3, LOW);
}
