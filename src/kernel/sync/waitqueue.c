#include "kernel/sync/waitqueue.h"
#include "kernel/sched/circ_queue.h"
#include "lib/stddef.h"
#include "lib/stdint.h"
#include "lib/string.h"
#include <kernel/sched/scheduler.h>

// Helpers for the waiting queue

void wq_init(wait_queue_t *wq) {
  wq->head = NULL;
  wq->tail = NULL;
}

const process_t *wq_peek_head(wait_queue_t *wq) { return wq->head; }

void wq_enqueue(process_t *proc, wait_queue_t *wq) {
  if (!wq->head) {
    wq->head = wq->tail = proc;
    set_next_wait(proc, NULL);
    set_prev_wait(proc, NULL);
  } else {
    set_next_wait(wq->tail, proc);
    set_prev_wait(proc, wq->tail);
    wq->tail = proc;
    set_next_wait(proc, NULL);
  }
}

/** Remove an element from a waint queue queue **/
void wq_remove(process_t *proc, wait_queue_t *wq) {
  process_t *prev = get_prev_wait(proc);
  process_t *next = get_next_wait(proc);
  if (prev) {
    set_next_wait(prev, next);
  } else {
    wq->head = next;
  }

  if (next) {
    set_prev_wait(next, prev);
  } else {
    wq->tail = prev;
  }

  if (is_in_sleeping_queue(proc))
    remove_from_sleeping(proc);

  // Remove it from current waiting queue
  set_next_wait(proc, NULL);
  set_prev_wait(proc, NULL);
}

/** Remove the last element of the waiting queue **/
process_t *wq_pop_head(wait_queue_t *wq) {
  process_t *head = wq->head;
  if (!head)
    return NULL;
  wq_remove(head, wq);
  return head;
}

/** Remove an item by pid, it returns the item if found else a NULL pointer **/
process_t *wq_remove_by_pid(wait_queue_t *wq, int8_t pid) {
  process_t *cur = wq->head;
  while (cur && get_pid(cur) != pid)
    cur = get_next_wait(cur);

  if (!cur)
    return NULL;

  wq_remove(cur, wq);
  return cur;
}

uint8_t wq_is_empty(wait_queue_t *wq) { return wq_peek_head(wq) == NULL; }

/* Pop all: atomically take the whole list and return its head.
   The caller becomes responsible for processing the returned list. */
process_t *wq_pop_all(wait_queue_t *wq) {
  process_t *head = wq->head;
  if (!head)
    return NULL;
  wq->head = wq->tail = NULL;

  /* clear owner pointers on the popped list will be done while iterating in
   * caller */
  return head;
}
