#include "programs.h"
#include "clist.h"
#include "cmd_registry.h"
#include "container.h"
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

static int _pretty_print_process(clist_node_t *node, void *args) {
  (void)args;

  process_t *proc = container_of(node, process_t, proc_node);
  process_t *parent = proc->parent;
  uint8_t pid = 0;
  if (parent) {
    pid = parent->pid;
  }
  printf("%-5d %-10d %-5s \n", proc->pid, pid, proc->name);
  return 0;
}
/**
 * @brief Temporary proc to list all of the current processus.
 */
static void ps() {
  printf("%-5s %-10s %-5s\n", "PID", "PPID", "CMD");
  clist_for_each(get_proc_table_clist(), _pretty_print_process, NULL);
}

/** Register the programs **/
void init_programs() {
  register_prog("proc1", proc1, HIGH);
  register_prog("proc2", proc2, NORMAL);
  register_prog("proc3", proc3, LOW);
  register_prog("ps", ps, NORMAL);
}
