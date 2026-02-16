#include "programs.h"
#include "cmd_registry.h"
#include "minilib/stddef.h"
#include "minilib/stdio.h"
#include "process.h"
#include "scheduler.h"
#include "time.h"

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
