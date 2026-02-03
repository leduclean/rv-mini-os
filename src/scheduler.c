#include "scheduler.h"
#include "process.h"
#include "time.h"
#include <stddef.h>
#include <stdint.h>

extern void ctx_sw(uintptr_t old_ctx, uintptr_t new_ctx);
static circ_queu_t run_queue = {0};

/** Circular run queue gestion **/
static inline process_t *peek_head(void) {
  if (run_queue.size == 0)
    return NULL;
  return run_queue.queue[run_queue.head];
}

/** Round Robin circular gestion of the run queue **/
static void rotate_head_to_tail(void) {
  // Nothing to do if 1 or  0 element
  if (run_queue.size <= 1)
    return;
  process_t *p = run_queue.queue[run_queue.head];
  run_queue.queue[run_queue.tail] = p;
  run_queue.tail = (run_queue.tail + 1) % MAX_PROC;
  run_queue.head = (run_queue.head + 1) % MAX_PROC;
}

/** Add a new process to the run_queue **/
int enqueue(process_t *proc) {
  if (run_queue.size >= MAX_PROC)
    return -1; // queue is full
  run_queue.queue[run_queue.tail] = proc;
  run_queue.tail = (run_queue.tail + 1) % MAX_PROC;
  run_queue.size++;
  return 0;
}

/** Remove the active process frome the run_queue **/
static int dequeue(void) {
  if (run_queue.size == 0)
    return -1; // Should not happend since idle is always here
  run_queue.head = (run_queue.head + 1) % MAX_PROC;
  run_queue.size--;
  return 0;
}

/** Sleeping queue handling **/
static process_t *sleeping_head = NULL;

static void insert_sleep(process_t *proc) {
  // If no sleeping head
  if (!sleeping_head || get_wake_up(proc) < get_wake_up(sleeping_head)) {
    set_next_sleeping(proc, sleeping_head);
    sleeping_head = proc;
    return;
  }
  process_t *prev = sleeping_head;
  process_t *current = get_next_sleeping(prev);

  while (current && get_wake_up(current) <= get_wake_up(proc)) {
    prev = current;
    current = get_next_sleeping(current);
  }

  set_next_sleeping(proc, current);
  set_next_sleeping(prev, proc);
}

static void do_ctx_switch(process_t *next) {
  uint64_t *old_ctx = get_ctx(get_active());
  switch_active(next);
  ctx_sw((uintptr_t)old_ctx, (uintptr_t)get_ctx(next));
}

/* Schedule function trigered by an interupt */
void scheduler_rotate() {
  rotate_head_to_tail();
  process_t *head = peek_head();
  if (head) {
    do_ctx_switch(peek_head());
  }
}

/* Schedule function trigered by an inactivity of the process
 * ie terminason, or sleep
 * */
static void switch_out_active() {
  // Remove the active process from the running queue
  dequeue();
  do_ctx_switch(peek_head());
}

/** Terminate a processus **/
void scheduler_terminate() {
  process_terminate();
  switch_out_active();
}

/* Launcher to handle launch and terminaison of a proc */
void proc_launcher(void proc()) {
  proc();
  scheduler_terminate();
}

/** Set a program to sleeping state **/
void scheduler_sleep(uint32_t nbr_secs) {
  process_sleep(nbr_secs);
  insert_sleep(get_active());
  switch_out_active();
}

/** Wake up a process and reschedule it in the running queue **/
static void scheduler_wake(process_t *proc) {
  process_wake(proc);
  enqueue(proc);
}

/** Wake up the process wakable in the sleeping queue **/
void wake_up_sleeping() {
  uint32_t now = seconds();
  while (sleeping_head && get_wake_up(sleeping_head) <= now) {
    process_t *next = get_next_sleeping(sleeping_head);
    scheduler_wake(sleeping_head);
    set_next_sleeping(sleeping_head, NULL);
    sleeping_head = next;
  }
}
