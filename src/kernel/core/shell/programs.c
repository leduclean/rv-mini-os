#include "programs.h"
#include "clist.h"
#include "cmd_registry.h"
#include "container.h"
#include "minilib/stddef.h"
#include "minilib/stdio.h"
#include "mutex.h"
#include "process.h"
#include "scheduler.h"
#include "time.h"

#define MAX_CMDS 64

/* --- Shared mutex for mutex demo --- */
static mutex_t shared_mutex;
static uint8_t mutex_initialized = 0;

static void ensure_mutex_init() {
  if (!mutex_initialized) {
    mutex_init(&shared_mutex);
    mutex_initialized = 1;
  }
}

/* --- Demo processes --- */

static void proc1() {
  printf("[proc1] started (pid=%d, priority=HIGH)\n", get_active_pid());
  for (int i = 0; i < 3; i++) {
    printf("[proc1] running at t=%us — sleeping 5s...\n", seconds());
    scheduler_sleep(5);
  }
  printf("[proc1] done, exiting\n");
}

static void proc3() {
  printf("[proc3] spawned dynamically by proc2 (pid=%d, priority=LOW)\n",
         get_active_pid());
  for (;;) {
    printf("[proc3] alive at t=%us — sleeping 4s\n", seconds());
    scheduler_sleep(4);
  }
}

static void proc2() {
  printf("[proc2] started (pid=%d, priority=NORMAL)\n", get_active_pid());
  for (int i = 0; i < 6; i++) {
    printf("[proc2] iteration %d at t=%us — sleeping 6s\n", i, seconds());
    if (i == 1) {
      printf("[proc2] spawning proc3 dynamically...\n");
      spawn_process(proc3, "proc3", LOW);
    }
    scheduler_sleep(6);
  }
  printf("[proc2] done, exiting\n");
}

/* mutex_holder: acquires the shared mutex, holds it for 5s, then releases */
static void mutex_holder() {
  ensure_mutex_init();
  printf("[holder] acquiring mutex...\n");
  mutex_lock(&shared_mutex);
  printf("[holder] mutex acquired! holding for 15s (run `ps` to see waiter "
         "BLOCKED)\n");
  scheduler_sleep(15);
  printf("[holder] releasing mutex\n");
  mutex_unlock(&shared_mutex);
  printf("[holder] done\n");
}

static void mutex_waiter() {
  ensure_mutex_init();
  printf("[waiter] trying to acquire mutex (will BLOCK if holder has it)...\n");
  mutex_lock(&shared_mutex);
  printf("[waiter] mutex acquired! (unblocked by holder's release)\n");
  mutex_unlock(&shared_mutex);
  printf("[waiter] done\n");
}

/* --- ps command --- */

static const char *state_str(state s) {
  switch (s) {
  case FREE:
    return "FREE";
  case RUNNING:
    return "RUN";
  case READY:
    return "READY";
  case SLEEPING:
    return "SLEEP";
  case BLOCKED:
    return "BLOCK";
  case TERMINATED:
    return "TERM";
  case ZOMBIE:
    return "ZOMBIE";
  default:
    return "?";
  }
}

static int _pretty_print_process(clist_node_t *node, void *args) {
  (void)args;
  process_t *proc = container_of(node, process_t, proc_node);
  process_t *parent = proc->parent;
  uint8_t ppid = parent ? parent->pid : 0;
  printf("%-5d %-5d %-10s %-7s\n", proc->pid, ppid, proc->name,
         state_str(proc->state));
  return 0;
}

static void ps() {
  printf("%-5s %-5s %-10s %-7s\n", "PID", "PPID", "CMD", "STATE");
  clist_for_each(get_proc_table_clist(), _pretty_print_process, NULL);
}

/* --- Register all programs --- */

void init_programs() {
  register_prog("proc1", proc1, HIGH);
  register_prog("proc2", proc2, NORMAL);
  register_prog("proc3", proc3, LOW);
  register_prog("ps", ps, NORMAL);
  register_prog("mutex-holder", mutex_holder, NORMAL);
  register_prog("mutex-waiter", mutex_waiter, NORMAL);
}
