#include "process.h"
#include "time.h"
#include <stddef.h>
#include <stdint.h>

extern void ctx_sw(uintptr_t old_ctx, uintptr_t new_ctx);

/** Queue gestion **/

static inline process_t *peek_head(void) {
  if (run_queue.size == 0)
    return NULL;
  return run_queue.queue[run_queue.head];
}

/** Round Robin circular gestion of the run queue **/
void rotate_head_to_tail(void) {
  // Nothing to do if 1 or  0 element
  if (run_queue.size <= 1)
    return;
  process_t *p = run_queue.queue[run_queue.head];
  run_queue.queue[run_queue.tail] = p;
  run_queue.tail = (run_queue.tail + 1) % PROC_TABLE_SIZE;
  run_queue.head = (run_queue.head + 1) % PROC_TABLE_SIZE;
}

/** Add a new process to the run_queue **/
int enqueue(process_t *proc) {
  if (run_queue.size >= PROC_TABLE_SIZE)
    return -1; // queue is full
  run_queue.queue[run_queue.tail] = proc;
  run_queue.tail = (run_queue.tail + 1) % PROC_TABLE_SIZE;
  run_queue.size++;
  return 0;
}

/** Remove the active process frome the run_queue **/
int dequeue(void) {
  if (run_queue.size == 0)
    return -1; // Should not happend since idle is always here
  run_queue.head = (run_queue.head + 1) % PROC_TABLE_SIZE;
  run_queue.size--;
  return 0;
}

static void do_ctx_switch(process_t *next) {
  if (active->state == RUNNING)
    active->state = READY;

  next->state = RUNNING;

  uintptr_t old_ctx = (uintptr_t)&active->ctx;
  active = next;
  ctx_sw(old_ctx, (uintptr_t)&next->ctx);
}

/* Schedule function trigered by an interupt */
void ordonnance() {
  rotate_head_to_tail();
  do_ctx_switch(peek_head());
}

/* Schedule function trigered by an inactivity of the process
 * ie terminason, or sleep
 * */
void switch_out_active() {
  // Remove the active process from the running queue
  dequeue();
  do_ctx_switch(peek_head());
}

/** Terminate a processus **/
void fin_processus() {
  active->state = TERMINATED;
  proc_table.active_process--;
  switch_out_active();
}

/* Launcher to handle launch and terminaison of a proc */
void proc_launcher(void proc()) {
  proc();
  fin_processus();
}

/** Set a program to sleeping state **/
void dors(uint64_t nbr_secs) {
  active->wake_up_time = nbr_secs + nbr_secondes();
  active->state = SLEEPING;
  switch_out_active();
}

/** Wake up process and put them in the run_queu **/
void wake_up_sleeping(void) {
  uint64_t now = nbr_secondes();

  for (uint8_t i = 0; i < PROC_TABLE_SIZE; i++) {
    process_t *p = &proc_table.table[i];

    if (p->state == SLEEPING && p->wake_up_time <= now) {
      p->state = READY;
      enqueue(p);
    }
  }
}
