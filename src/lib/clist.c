#include <stdint.h>

#include <lib/clist.h>

void clist_init_node(clist_node_t *node)
{
	node->prev = node->next = node;
	node->in_list = 0;
}

uint8_t clist_empty(const clist_node_t *head)
{
	return head->next == head;
}

void clist_insert_before(clist_node_t *pos, clist_node_t *node)
{
	if (node->in_list)
		return;
	node->next = pos;
	node->prev = pos->prev;
	pos->prev->next = node;
	pos->prev = node;
	node->in_list = 1;
}

void clist_insert_after(clist_node_t *pos, clist_node_t *node)
{
	if (node->in_list)
		return;
	node->next = pos->next;
	pos->next->prev = node;
	node->prev = pos;
	pos->next = node;
	node->in_list = 1;
}

void clist_push_back(clist_node_t *head, clist_node_t *node)
{
	clist_insert_before(head, node);
}

void clist_push_front(clist_node_t *head, clist_node_t *node)
{
	clist_insert_after(head, node);
}

void clist_remove(clist_node_t *node)
{
	if (!node || !node->in_list)
		return;
	node->prev->next = node->next;
	node->next->prev = node->prev;
	clist_init_node(node); // Reset the node
}

clist_node_t *clist_first(clist_node_t *head)
{
	return clist_empty(head) ? NULL : head->next;
}

clist_node_t *clist_last(clist_node_t *head)
{
	return clist_empty(head) ? NULL : head->prev;
}

uint8_t clist_is_in_list(const clist_node_t *node)
{
	return node->in_list;
}

clist_node_t *clist_for_each(const clist_node_t *head,
			     int (*callback)(clist_node_t *node, void *),
			     void *args)
{
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

void clist_insert_sorted(clist_node_t *head, clist_node_t *node,
			 clist_cmp_func_t cmp)
{
	clist_node_t *current = head->next;
	while ((current != head) && cmp(current, node) <= 0) {
		current = current->next;
	}
	clist_insert_before(current, node);
}

clist_node_t *clist_find(clist_node_t *head,
			 int (*pred)(clist_node_t *node, void *), void *args)
{
	clist_node_t *current = head->next;
	while (current != head) {
		if (pred(current, args)) {
			return current;
		}
		current = current->next;
	}
	return NULL;
}

clist_node_t *
clist_for_each_and_del(clist_node_t *head,
		       int (*callback)(clist_node_t *node, void *), void *args)
{
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

clist_node_t *clist_pop_front(clist_node_t *head)
{
	if (clist_empty(head))
		return NULL;
	clist_node_t *first = head->next;
	clist_remove(first);
	return first;
}
