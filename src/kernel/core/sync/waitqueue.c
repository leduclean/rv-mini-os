#include <stddef.h>
#include <stdint.h>

#include <lib/clist.h>
#include <lib/container.h>
#include <lib/string.h>

#include <kernel/process.h>
#include <kernel/waitqueue.h>

// Helpers for the waiting queue
void wq_init(wait_queue_t *wq)
{
	clist_init_node(&wq->head);
}

const clist_node_t *wq_peek_head(wait_queue_t *wq)
{
	return clist_first(&wq->head);
}

void wq_enqueue(wait_queue_t *wq, clist_node_t *node)
{
	clist_push_back(&wq->head, node);
}

void wq_remove(clist_node_t *node)
{
	clist_remove(node);
}

clist_node_t *wq_pop_head(wait_queue_t *wq)
{
	return clist_pop_front(&wq->head);
}

uint8_t wq_is_empty(const wait_queue_t *wq)
{
	return clist_empty(&wq->head);
}

clist_node_t *wq_for_each(wait_queue_t *wq,
			  int (*func)(clist_node_t *node, void *), void *args)
{
	return clist_for_each(&wq->head, func, args);
}

clist_node_t *wq_for_each_and_del(wait_queue_t *wq,
				  int (*fun)(clist_node_t *node, void *),
				  void *args)
{
	return clist_for_each_and_del(&wq->head, fun, args);
}
