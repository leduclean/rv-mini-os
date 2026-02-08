#pragma once
#include "lib/stdint.h"

typedef struct process process_t;

typedef struct wait_queue {
  process_t *head;
  process_t *tail;
} wait_queue_t;

// Helpers for the waiting queue
void wq_init(wait_queue_t *wq);
const process_t *wq_peek_head(wait_queue_t *wq);
void wq_enqueue(process_t *proc, wait_queue_t *wq);
process_t *wq_pop_head(wait_queue_t *wq);
void wq_remove(process_t *proc, wait_queue_t *wq);
uint8_t wq_is_empty(wait_queue_t *wq);
process_t *wq_remove_by_pid(wait_queue_t *wq, int8_t pid);
process_t *wq_pop_all(wait_queue_t *wq);
