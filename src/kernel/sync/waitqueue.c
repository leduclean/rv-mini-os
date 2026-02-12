#include "kernel/sync/waitqueue.h"
#include "kernel/process/process.h"
#include "lib/clist.h"
#include "lib/container.h"
#include "lib/stddef.h"
#include "lib/stdint.h"
#include "lib/string.h"

// Helpers for the waiting queue
void wq_init(wait_queue_t *wq) { clist_init_node(&wq->head); }

const clist_node_t *wq_peek_head(wait_queue_t *wq) {
  return clist_first(&wq->head);
}

void wq_enqueue(wait_queue_t *wq, clist_node_t *node) {
  clist_push_back(&wq->head, node);
}

/** Remove an element from a waiting queue **/
void wq_remove(clist_node_t *node) { clist_remove(node); }

/** Remove first element of the waiting queue **/
clist_node_t *wq_pop_head(wait_queue_t *wq) {
  return clist_pop_front(&wq->head);
}

/** Remove an item by pid, it returns the item if found else a NULL pointer **/
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

uint8_t wq_is_empty(wait_queue_t *wq) { return clist_empty(&wq->head); }

clist_node_t *wq_for_each(wait_queue_t *wq,
                          int (*func)(clist_node_t *node, void *), void *args) {
  return clist_for_each(&wq->head, func, args);
}

clist_node_t *wq_for_each_and_del(wait_queue_t *wq,
                                  int (*fun)(clist_node_t *node, void *),
                                  void *args) {
  return clist_for_each_and_del(&wq->head, fun, args);
}
