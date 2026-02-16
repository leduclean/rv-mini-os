#include "clist.h"
#include "minilib/stdint.h"

/**
 * @brief Init a node list.
 *
 * @note The invariant init is to self point. You always have to init the clist
 * sentinel.
 *
 * @param node Pointer of the node to init
 */
void clist_init_node(clist_node_t *node) {
  node->prev = node->next = node;
  node->in_list = 0;
}

/**
 * @brief Empty indicator of the clist.
 *
 * @param head Head sentinel pointer of the clist.
 * @return 1 if empty else 0.
 */
uint8_t clist_empty(clist_node_t *head) { return head->next == head; }

/**
 * @brief Insert a node before another in the clist.
 *
 * @param pos Pointer of the pos to insert before in the clist.
 * @param node Pointer to the item to insert in the clist.
 */
void clist_insert_before(clist_node_t *pos, clist_node_t *node) {
  if (node->in_list)
    return;
  node->next = pos;
  node->prev = pos->prev;
  pos->prev->next = node;
  pos->prev = node;
  node->in_list = 1;
}

/**
 * @brief Insert a node after another in the clist.
 *
 * @param pos Pointer of the pos to insert after in the clist.
 * @param node Pointer to the item to insert in the clist.
 */
void clist_insert_after(clist_node_t *pos, clist_node_t *node) {
  if (node->in_list)
    return;
  node->next = pos->next;
  pos->next->prev = node;
  node->prev = pos;
  pos->next = node;
  node->in_list = 1;
}

/**
 * @brief Add an item to the end of the clist.
 *
 * @param head Head sentinel pointer of the clist.
 * @param node Pointer to the node item to add.
 */
void clist_push_back(clist_node_t *head, clist_node_t *node) {
  clist_insert_before(head, node);
}

/**
 * @brief Remove a node in the list if his is suposed to be in.
 *
 * @note You can't remove head sentinel since it as always in_list set to false.
 * @param node A pointer on the node to remove from the list.
 */
void clist_remove(clist_node_t *node) {
  if (!node || !node->in_list)
    return;
  node->prev->next = node->next;
  node->next->prev = node->prev;
  clist_init_node(node); // Reset the node
}

/**
 * @brief Get the first element of the clist.
 *
 * @param head Head sentinel of the clist
 * @return Pointer to the first element.
 */
clist_node_t *clist_first(clist_node_t *head) {
  return clist_empty(head) ? NULL : head->next;
}

/**
 * @brief Get the last item from the clist.
 *
 * @param head Head sentinel of the clist.
 * @return Pointer to the last item.
 */
clist_node_t *clist_last(clist_node_t *head) {
  return clist_empty(head) ? NULL : head->prev;
}

/**
 * @brief Check if a node is in a list.
 *
 * @param node
 * @return 1 if in else 0.
 */
uint8_t clist_is_in_list(clist_node_t *node) { return node->in_list; }

/**
 * @brief Traverse the clist, call the function to each mumber.
 *
 * @param head List sentinel to traverse.
 * @param func Callback to call for each member.
 * @param node Pointer to pass to every call of the funciton.
 *
 * @return NULL on empty list or full traversal
 * @return node that cause @p func(node) to exit non-zero.
 */
clist_node_t *clist_for_each(const clist_node_t *head,
                             int (*callback)(clist_node_t *node, void *),
                             void *args) {
  clist_node_t *current = head->next;
  while (current != head) {
    clist_node_t *next = current->next;
    if (callback(current, args)) {
      return current;
    }
    current = next;
  }
  return NULL;
}

/**
 * @brief Insert a node at the good position using a cmp policy func.
 *
 * @param head Head sentinel of the list.
 * @param node Node to Insert.
 * @param cmp Comparison policy func.
 */
void clist_insert_sorted(clist_node_t *head, clist_node_t *node,
                         clist_cmp_func_t cmp) {
  clist_node_t *current = head->next;
  while ((current != head) && cmp(current, node) <= 0) {
    current = current->next;
  }
  clist_insert_before(current, node);
}

/**
 * @brief Find the first element in the clist matching the predicate.
 *
 * @param head Head sentinel pointer of the clist.
 * @param pred Predicate to find.
 * @return Null if not found else the pointer of the matching node.
 */
clist_node_t *clist_find(clist_node_t *head,
                         int (*pred)(clist_node_t *node, void *), void *args) {
  clist_node_t *current = head->next;
  while (current != head) {
    if (pred(current, args)) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

/**
 * @brief Apply a function to each elements of the clist and detach them
 * recursively.
 *
 * @note In any case all the element are going to be detached but callback will
 * stop on the first error
 *
 *
 * @param head The head sentinel of the clist.
 * @param fun The callback to apply on the clist.
 *
 * @return NULL if all successfull. The node pointer on which the function
 * failed.
 */
clist_node_t *clist_for_each_and_del(clist_node_t *head,
                                     int (*callback)(clist_node_t *node,
                                                     void *),
                                     void *args) {
  clist_node_t *current = head->next;
  clist_node_t *failure = NULL;
  clist_init_node(head); // reset sentinel
  while (current != head) {
    clist_node_t *next = current->next;
    clist_remove(current);
    if (callback(current, args)) {
      failure = failure ? failure : current;
    }
    current = next;
  }
  return failure;
}

/**
 * @brief Removes and return the first element from the clist
 *
 * @param head Head sentinel of the list.
 * @return Pointer to the first element.
 */
clist_node_t *clist_pop_front(clist_node_t *head) {
  if (clist_empty(head))
    return NULL;
  clist_node_t *first = head->next;
  clist_remove(first);
  return first;
}
