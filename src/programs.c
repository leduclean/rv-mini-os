#include "programs.h"
#include "process.h"
#include "scheduler.h"
#include "time.h"
#include <stddef.h>
#include <stdio.h>

void proc1() {
  for (int i = 0; i < 2; i++) {
    printf("[temps = %u] processus %s pid = %i\n", seconds(), get_active_name(),
           get_active_pid());
    scheduler_sleep(2);
  }
}

void proc3() {
  for (;;) {
    printf("[temps = %u] processus %s pid = %i\n", seconds(), get_active_name(),
           get_active_pid());
    scheduler_sleep(1);
  }
}

void proc2() {
  for (int i = 0; i < 10; i++) {
    printf("[temps = %u] processus %s pid = %i\n", seconds(), get_active_name(),
           get_active_pid());
    if (i == 1) {
      spawn_process(proc3, "bob3", HIGH);
    }
    scheduler_sleep(3);
  }
}
