/**
 * @file
 * @brief Circular doubly linked list
 *
 * This file contains a circularly and doubly linked list implementaion
 *
 * This clist can be used as a traditional list, a queue (FIFO) and a stack
 * (LIFO) using O(1).
 *
 * Implementation details:
 *
 * Each list is represented as a "clist_node_t" sentinel.
 * Its first member, the "next" pointer points to the first element of the list.
 * Whereas its second element the "prev" pointer points to the last element of
 * the list.
 *
 *
 */

#pragma once

#if TEST_CONFIG
#include <stddef.h>
#include <stdint.h>
#else
#include "lib/stddef.h"
#include "lib/stdint.h"
#endif

typedef struct clist_node {
  struct clist_node *next;
  struct clist_node *prev;
  uint8_t in_list; // 0 si hors liste, 1 si dans liste
} clist_node_t;

typedef int (*clist_cmp_func_t)(clist_node_t *a, clist_node_t *b);

void clist_init_node(clist_node_t *node);

uint8_t clist_empty(clist_node_t *head);
clist_node_t *clist_first(clist_node_t *head);
clist_node_t *clist_last(clist_node_t *head);
void clist_insert_before(clist_node_t *pos, clist_node_t *node);
void clist_insert_after(clist_node_t *pos, clist_node_t *node);
void clist_insert_sorted(clist_node_t *head, clist_node_t *node,
                         clist_cmp_func_t cmp);

clist_node_t *clist_find(clist_node_t *head,
                         int (*pred)(clist_node_t *node, void *), void *args);

clist_node_t *clist_for_each(clist_node_t *head,
                             int (*func)(clist_node_t *node, void *),
                             void *args);

clist_node_t *clist_for_each_and_del(clist_node_t *head,
                                     int (*fun)(clist_node_t *node, void *),
                                     void *args);
void clist_push_back(clist_node_t *head, clist_node_t *node);
void clist_remove(clist_node_t *node);
clist_node_t *clist_pop_front(clist_node_t *head);
uint8_t clist_is_in_list(clist_node_t *node);
