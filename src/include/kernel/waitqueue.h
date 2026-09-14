/**
 * @file
 * @brief Queue of processes blocked on a same condition.
 */

#pragma once
#include <stdint.h>

#include <lib/clist.h>

/** @brief Forward declaration of the process control block. */
typedef struct process process_t;

/** @brief Wait queue, a clist of process wait nodes. */
typedef struct wait_queue {
	clist_node_t head; ///< Head sentinel of the waiting nodes clist.
} wait_queue_t;

/**
 * @brief Init a wait queue.
 *
 * @param wq Wait queue to init.
 */
void wq_init(wait_queue_t *wq);

/**
 * @brief Get the first waiting node without detaching it.
 *
 * @param wq Wait queue to read.
 * @return Pointer to the first waiting node, NULL if @p wq is empty.
 */
const clist_node_t *wq_peek_head(wait_queue_t *wq);

/**
 * @brief Append a node at the end of a wait queue.
 *
 * @param wq Wait queue to append to.
 * @param node Wait node of the process to enqueue.
 */
void wq_enqueue(wait_queue_t *wq, clist_node_t *node);

/**
 * @brief Remove a node from the wait queue it belongs to.
 *
 * @param node Wait node to detach.
 */
void wq_remove(clist_node_t *node);

/**
 * @brief Remove and return the first node of a wait queue.
 *
 * @param wq Wait queue to pop from.
 * @return Pointer to the detached node, NULL if @p wq is empty.
 */
clist_node_t *wq_pop_head(wait_queue_t *wq);

/**
 * @brief Empty indicator of a wait queue.
 *
 * @param wq Wait queue to read.
 * @return 1 if empty, 0 otherwise.
 */
uint8_t wq_is_empty(const wait_queue_t *wq);

/**
 * @brief Remove a waiting process by pid.
 *
 * @param wq Wait queue to search in.
 * @param pid Pid of the process to detach.
 * @return Pointer to the detached process, NULL if it was not waiting here.
 */
process_t *wq_remove_by_pid(wait_queue_t *wq, int8_t pid);

/**
 * @brief Traverse a wait queue, calling a function on each waiting node.
 *
 * @param wq Wait queue to traverse.
 * @param func Callback called for each waiting node.
 * @param args Argument forwarded to every @p func call.
 * @return Node that caused @p func to exit non-zero, NULL on a full
 * traversal.
 */
clist_node_t *wq_for_each(wait_queue_t *wq,
			  int (*func)(clist_node_t *node, void *), void *args);

/**
 * @brief Apply a function to each waiting node and detach them all.
 *
 * @param wq Wait queue to drain.
 * @param fun Callback applied to each detached node.
 * @param args Argument forwarded to every @p fun call.
 * @return Node on which @p fun failed, NULL if all succeeded.
 */
clist_node_t *wq_for_each_and_del(wait_queue_t *wq,
				  int (*fun)(clist_node_t *node, void *),
				  void *args);
