#include "circ_queue.h"
#include "process.h"
#include <stddef.h>
#include <stdint.h>

/** Get the active process **/
inline process_t *peek_head(circ_queue_t *q) {
  if (q->size == 0)
    return NULL;
  return q->queue[q->head];
}

/** Round Robin circular gestion of a queue **/
void rotate_head_to_tail(circ_queue_t *q) {
  // Nothing to do if 0 or 1 element
  if (q->size <= 1)
    return;

  process_t *p = q->queue[q->head];
  q->queue[q->tail] = p;

  q->tail = (q->tail + 1) % MAX_PROC;
  q->head = (q->head + 1) % MAX_PROC;
}

/** Add a new process to a circular queue **/
int enqueue(circ_queue_t *q, process_t *proc) {
  if (q->size >= MAX_PROC)
    return -1; // queue is full

  q->queue[q->tail] = proc;
  q->tail = (q->tail + 1) % MAX_PROC;
  q->size++;

  return 0;
}

/** Remove the head process from a circular queue **/
int dequeue(circ_queue_t *q) {
  if (q->size == 0)
    return -1;

  q->head = (q->head + 1) % MAX_PROC;
  q->size--;

  return 0;
}

/** Remove and get the head process from the queue **/
process_t *pop(circ_queue_t *q) {
  process_t *proc = peek_head(q);
  dequeue(q);
  return proc;
}

uint8_t is_empty(circ_queue_t *q) { return q->size == 0; }

/** Remove an item by pid, it returns the item if found else a NULL pointer **/
process_t *circ_remove_by_pid(circ_queue_t *q, int8_t pid) {
  process_t *found = NULL;
  if (q->size == 0)
    return NULL;

  for (int8_t i = 0; i < q->size; i++) {
    if (get_pid(q->queue[i]) == pid) {
      found = q->queue[i];
      // Move all the element after this element
      int8_t next = i + 1 % MAX_PROC;
      while (next != q->tail) {
        q->queue[i] = q->queue[next];
        i = next;
        next++;
      }
      break;
    }
  }
  return found;
}
