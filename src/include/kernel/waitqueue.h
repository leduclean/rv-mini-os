#pragma once
#include "clist.h"
#include "kernel_config.h"

#if TEST_CONFIG
#include <stdint.h>
#else
#include "minilib/stdint.h"
#endif

typedef struct process process_t;

typedef struct wait_queue {
  clist_node_t head;
} wait_queue_t;

// Helpers for the waiting queue
void wq_init(wait_queue_t *wq);
const clist_node_t *wq_peek_head(wait_queue_t *wq);
void wq_enqueue(wait_queue_t *wq, clist_node_t *node);
void wq_remove(clist_node_t *node);
clist_node_t *wq_pop_head(wait_queue_t *wq);
uint8_t wq_is_empty(wait_queue_t *wq);
process_t *wq_remove_by_pid(wait_queue_t *wq, int8_t pid);

clist_node_t *wq_for_each(wait_queue_t *wq,
                          int (*func)(clist_node_t *node, void *), void *args);

clist_node_t *wq_for_each_and_del(wait_queue_t *wq,
                                  int (*fun)(clist_node_t *node, void *),
                                  void *args);
