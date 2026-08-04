/**
 * @file
 * @brief Circular doubly linked list.
 *
 * Each list is represented as a "clist_node_t" sentinel. Its first member,
 * the "next" pointer, points to the first element of the list, whereas its
 * second member, the "prev" pointer, points to the last one.
 *
 * This clist can be used as a traditional list, a queue (FIFO) and a stack
 * (LIFO) in O(1).
 */

#pragma once

#if TEST_CONFIG
#include <stddef.h>
#include <stdint.h>
#else
#include "minilib/stddef.h"
#include "minilib/stdint.h"
#endif

/** @brief Node of a clist, embedded in the owning structure. */
typedef struct clist_node {
  struct clist_node *next; ///< Next node, the sentinel itself if last.
  struct clist_node *prev; ///< Previous node, the sentinel itself if first.
  uint8_t in_list;         ///< 1 if the node is in a clist, 0 otherwise.
} clist_node_t;

/**
 * @brief Comparison policy used to keep a clist sorted.
 *
 * @param a First node to compare.
 * @param b Second node to compare.
 * @return A value greater than 0 if @p a must be placed after @p b.
 */
typedef int (*clist_cmp_func_t)(clist_node_t *a, clist_node_t *b);

/**
 * @brief Init a node.
 *
 * @note The invariant init is to self point. You always have to init the
 * clist sentinel.
 *
 * @param node Pointer to the node to init.
 */
void clist_init_node(clist_node_t *node);

/**
 * @brief Empty indicator of the clist.
 *
 * @param head Head sentinel of the clist.
 * @return 1 if empty, 0 otherwise.
 */
uint8_t clist_empty(const clist_node_t *head);

/**
 * @brief Get the first element of the clist.
 *
 * @param head Head sentinel of the clist.
 * @return Pointer to the first element, NULL if the clist is empty.
 */
clist_node_t *clist_first(clist_node_t *head);

/**
 * @brief Get the last element of the clist.
 *
 * @param head Head sentinel of the clist.
 * @return Pointer to the last element, NULL if the clist is empty.
 */
clist_node_t *clist_last(clist_node_t *head);

/**
 * @brief Insert a node before another one in the clist.
 *
 * @param pos Node to insert before.
 * @param node Node to insert.
 */
void clist_insert_before(clist_node_t *pos, clist_node_t *node);

/**
 * @brief Insert a node after another one in the clist.
 *
 * @param pos Node to insert after.
 * @param node Node to insert.
 */
void clist_insert_after(clist_node_t *pos, clist_node_t *node);

/**
 * @brief Insert a node at the position given by a comparison policy.
 *
 * @param head Head sentinel of the clist.
 * @param node Node to insert.
 * @param cmp Comparison policy function.
 */
void clist_insert_sorted(clist_node_t *head, clist_node_t *node,
                         clist_cmp_func_t cmp);

/**
 * @brief Find the first element of the clist matching a predicate.
 *
 * @param head Head sentinel of the clist.
 * @param pred Predicate applied to each node.
 * @param args Argument forwarded to every @p pred call.
 * @return Pointer to the matching node, NULL if none matched.
 */
clist_node_t *clist_find(clist_node_t *head,
                         int (*pred)(clist_node_t *node, void *), void *args);

/**
 * @brief Traverse the clist, calling a function on each member.
 *
 * @param head Head sentinel of the clist to traverse.
 * @param func Callback called for each member.
 * @param args Argument forwarded to every @p func call.
 * @return Node that caused @p func to exit non-zero.
 * @return NULL on an empty clist or a full traversal.
 */
clist_node_t *clist_for_each(const clist_node_t *head,
                             int (*func)(clist_node_t *node, void *),
                             void *args);

/**
 * @brief Apply a function to each element of the clist and detach them.
 *
 * @note In any case all the elements are going to be detached, but the
 * returned node is the first one whose callback failed.
 *
 * @param head Head sentinel of the clist.
 * @param fun Callback applied to each detached member.
 * @param args Argument forwarded to every @p fun call.
 * @return Node on which @p fun failed, NULL if all succeeded.
 */
clist_node_t *clist_for_each_and_del(clist_node_t *head,
                                     int (*fun)(clist_node_t *node, void *),
                                     void *args);

/**
 * @brief Add an element at the end of the clist.
 *
 * @param head Head sentinel of the clist.
 * @param node Node to add.
 */
void clist_push_back(clist_node_t *head, clist_node_t *node);

/**
 * @brief Remove a node from the clist it belongs to.
 *
 * @note You can't remove a head sentinel since it always has in_list set to
 * false.
 *
 * @param node Node to detach.
 */
void clist_remove(clist_node_t *node);

/**
 * @brief Remove and return the first element of the clist.
 *
 * @param head Head sentinel of the clist.
 * @return Pointer to the detached first element, NULL if the clist is empty.
 */
clist_node_t *clist_pop_front(clist_node_t *head);

/**
 * @brief Check if a node belongs to a clist.
 *
 * @param node Node to check.
 * @return 1 if it is in a clist, 0 otherwise.
 */
uint8_t clist_is_in_list(const clist_node_t *node);
