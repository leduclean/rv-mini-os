#include "scheduler.h"
#include "process.h"
#include <stddef.h>
#include <stdint.h>

extern void ctx_sw(uintptr_t old_ctx, uintptr_t new_ctx);
static circ_queu_t run_queue = {0};

/** Queue gestion **/
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

static void do_ctx_switch(process_t *next) {
  uint64_t *old_ctx = get_ctx(get_active());
  switch_active(next);
  ctx_sw((uintptr_t)old_ctx, (uintptr_t)get_ctx(next));
}

/* Schedule function trigered by an interupt */
void scheduler_rotate() {
  rotate_head_to_tail();
  do_ctx_switch(peek_head());
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
  switch_out_active();
}

/** Wake up a process and reschedule it in the running queue **/
static void scheduler_wake(process_t *proc) {
  process_wake(proc);
  enqueue(proc);
}

/** Try to  wake up a single prog **/
static void try_wake_up(process_t *proc, uint32_t now) {
  if (get_wake_up(proc) <= now) {
    scheduler_wake(proc);
  }
}
