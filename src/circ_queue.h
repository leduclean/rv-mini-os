#pragma once
#include "process.h"

/** ROUND ROBING circular queue structure and interface **/
typedef struct {
  process_t *queue[MAX_PROC];
  uint8_t head; // Idx active
  uint8_t tail; // Idx for next insertion
  uint8_t size; // number of active element
} circ_queue_t;
/** Get the active process **/
extern process_t *peek_head(circ_queue_t *q);

/** Round Robin circular gestion of a queue **/
extern void rotate_head_to_tail(circ_queue_t *q);

/** Add a new process to a circular queue **/
extern int enqueue(circ_queue_t *q, process_t *proc);

/** Remove the head process from a circular queue **/
extern int dequeue(circ_queue_t *q);
extern process_t *pop(circ_queue_t *q);
extern uint8_t is_empty(circ_queue_t *q);
