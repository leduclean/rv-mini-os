#include "waitqueue.h"
#include "clist.h"
#include "container.h"
#include "process.h"
#include "minilib/stddef.h"
#include "minilib/stdint.h"
#include "minilib/string.h"

// Helpers for the waiting queue
void wq_init(wait_queue_t *wq) { clist_init_node(&wq->head); }

const clist_node_t *wq_peek_head(wait_queue_t *wq) {
  return clist_first(&wq->head);
}

void wq_enqueue(wait_queue_t *wq, clist_node_t *node) {
  clist_push_back(&wq->head, node);
}

void wq_remove(clist_node_t *node) { clist_remove(node); }

clist_node_t *wq_pop_head(wait_queue_t *wq) {
  return clist_pop_front(&wq->head);
}

process_t *wq_remove_by_pid(wait_queue_t *wq, int8_t pid) {
  clist_node_t *n;
  for (n = wq->head.next; n != &wq->head; n = n->next) {
    process_t *p = container_of(n, process_t, wait_node);
    if (get_pid(p) == pid) {
      clist_remove(n);
      return p;
    }
  }
  return NULL;
}

uint8_t wq_is_empty(const wait_queue_t *wq) { return clist_empty(&wq->head); }

clist_node_t *wq_for_each(wait_queue_t *wq,
                          int (*func)(clist_node_t *node, void *), void *args) {
  return clist_for_each(&wq->head, func, args);
}

clist_node_t *wq_for_each_and_del(wait_queue_t *wq,
                                  int (*fun)(clist_node_t *node, void *),
                                  void *args) {
  return clist_for_each_and_del(&wq->head, fun, args);
}
