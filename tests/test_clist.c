/**
 * @file
 * @brief Unit testing for the clist.
 */

#include "lib/clist.h"
#include "unity/unity.h"
#include "unity/unity_internals.h"
#include <stdint.h>
#include <stdlib.h>

void setUp() {}
void tearDown() {}

clist_node_t *create_list() {
  clist_node_t *list = malloc(sizeof(clist_node_t));
  clist_init_node(list);
  return list;
}
clist_node_t *create_node() {
  clist_node_t *node = malloc(sizeof(clist_node_t));
  clist_init_node(node);
  return node;
}

void test_clist_init() {
  clist_node_t *node = malloc(sizeof(clist_node_t));
  clist_init_node(node);
  TEST_ASSERT_EQUAL_PTR(node, node->prev);
  TEST_ASSERT_EQUAL_PTR(node, node->next);
  TEST_ASSERT_EQUAL_INT(0, node->in_list);
  free(node);
}

void test_clist_empty() {
  clist_node_t *empty = create_list();
  TEST_ASSERT_EQUAL_INT(1, clist_empty(empty));
  free(empty);
}

void test_clist_insert_before() {
  clist_node_t *list = create_list();
  clist_node_t *node1 = create_node();
  clist_node_t *node2 = create_node();

  clist_insert_before(list, node1);
  clist_insert_before(node1, node2);

  TEST_ASSERT_EQUAL_INT(node1->in_list, 1);
  TEST_ASSERT_EQUAL_INT(node2->in_list, 1);

  TEST_ASSERT_EQUAL_PTR(list->prev, node1);
  TEST_ASSERT_EQUAL_PTR(node1->prev, node2);

  TEST_ASSERT_EQUAL_PTR(list->next, node2);
  TEST_ASSERT_EQUAL_PTR(node1->next, list);

  free(list);
  free(node1);
  free(node2);
}

void test_clist_insert_after() {
  clist_node_t *list = create_list();
  clist_node_t *node1 = create_node();
  clist_node_t *node2 = create_node();

  clist_insert_after(list, node1);
  clist_insert_after(node1, node2);

  TEST_ASSERT_EQUAL_INT(node1->in_list, 1);
  TEST_ASSERT_EQUAL_INT(node2->in_list, 1);

  TEST_ASSERT_EQUAL_PTR(list->next, node1);
  TEST_ASSERT_EQUAL_PTR(node1->next, node2);

  TEST_ASSERT_EQUAL_PTR(list->prev, node2);
  TEST_ASSERT_EQUAL_PTR(node1->prev, list);

  free(list);
  free(node1);
  free(node2);
}

void test_clist_push_back() {
  clist_node_t *list = create_list();
  clist_node_t *node1 = create_node();
  clist_node_t *node2 = create_node();

  clist_push_back(list, node1);
  clist_push_back(list, node2);

  TEST_ASSERT_EQUAL_INT(node1->in_list, 1);
  TEST_ASSERT_EQUAL_INT(node2->in_list, 1);

  TEST_ASSERT_EQUAL_PTR(list->next, node1);
  TEST_ASSERT_EQUAL_PTR(node1->next, node2);

  TEST_ASSERT_EQUAL_PTR(list->prev, node2);
  TEST_ASSERT_EQUAL_PTR(node1->prev, list);

  free(list);
  free(node1);
  free(node2);
}

void test_clist_is_in_list() {
  clist_node_t *list = create_list();
  clist_node_t *node1 = create_node();

  clist_push_back(list, node1);
  TEST_ASSERT_EQUAL_UINT8(1, clist_is_in_list(node1));

  clist_node_t *node2 = create_node();
  TEST_ASSERT_EQUAL_UINT8(0, clist_is_in_list(node2));
}

void test_clist_first() {
  clist_node_t *list = create_list();
  clist_node_t *node = create_node();

  TEST_ASSERT_NULL(clist_first(list));

  clist_push_back(list, node);

  TEST_ASSERT_EQUAL_PTR(clist_first(list), node);

  free(list);
  free(node);
}

void test_clist_last() {
  clist_node_t *list = create_list();

  TEST_ASSERT_NULL(clist_last(list));

  clist_node_t *node1 = create_node();
  clist_node_t *node2 = create_node();

  clist_push_back(list, node1);
  clist_push_back(list, node2);

  TEST_ASSERT_EQUAL_PTR(clist_last(list), node2);

  free(list);
  free(node1);
  free(node2);
}

void test_clist_remove() {
  clist_node_t *list = create_list();
  clist_remove(list);

  TEST_ASSERT_EQUAL_INT(clist_empty(list), 1);

  clist_node_t *node1 = create_node();
  clist_node_t *node2 = create_node();

  clist_push_back(list, node1);
  clist_push_back(list, node2);

  clist_remove(node1);

  TEST_ASSERT_EQUAL_PTR(clist_first(list), node2);
  TEST_ASSERT_EQUAL_PTR(node1->prev, node1);
  TEST_ASSERT_EQUAL_PTR(node1->next, node1);

  free(list);
  free(node1);
  free(node2);
}

void test_clist_pop_front() {
  clist_node_t *list = create_list();

  TEST_ASSERT_NULL(clist_pop_front(list));

  clist_node_t *node1 = create_node();

  clist_push_back(list, node1);

  TEST_ASSERT_EQUAL_PTR(clist_pop_front(list), node1);

  free(list);
  free(node1);
}

static inline int callback_cnt(clist_node_t *node, void *args) {
  uint8_t *cnt = (uint8_t *)(args);
  (*cnt)++;
  return 0;
};

static inline int callback_fail_on_first(clist_node_t *node, void *args) {
  return -1;
};

void test_clist_for_each() {
  clist_node_t *list = create_list();
  clist_node_t *node1 = create_node();
  clist_node_t *node2 = create_node();

  clist_push_back(list, node1);
  clist_push_back(list, node2);

  TEST_ASSERT_EQUAL_PTR(clist_for_each(list, callback_fail_on_first, NULL),
                        node1);

  uint8_t cnt = 0;

  clist_node_t *result = clist_for_each(list, callback_cnt, &cnt);

  TEST_ASSERT_EQUAL_UINT8(2, cnt);
  TEST_ASSERT_NULL(result);

  free(list);
  free(node1);
  free(node2);
}

static inline int get_specific_node(clist_node_t *node, void *args) {
  clist_node_t *matching = (clist_node_t *)args;
  return matching == node;
}

void test_clist_find() {
  clist_node_t *list = create_list();
  clist_node_t *node1 = create_node();
  clist_node_t *node2 = create_node();

  clist_push_back(list, node1);

  // No match case
  TEST_ASSERT_NULL(clist_find(list, get_specific_node, node2));

  // Match case
  TEST_ASSERT_EQUAL_PTR(clist_find(list, get_specific_node, node1), node1);

  free(list);
  free(node1);
  free(node2);
}

static int cmp_nodes(clist_node_t *a, clist_node_t *b) {
  return ((int)(uintptr_t)a) - ((int)(uintptr_t)b);
}

void test_clist_insert_sorted(void) {
  clist_node_t *head = create_list();
  clist_node_t *node1 = create_node();
  clist_node_t *node2 = create_node();
  clist_node_t *node3 = create_node();

  clist_insert_sorted(head, node2, cmp_nodes);
  clist_insert_sorted(head, node1, cmp_nodes);
  clist_insert_sorted(head, node3, cmp_nodes);

  TEST_ASSERT_EQUAL_INT(1, node1->in_list);
  TEST_ASSERT_EQUAL_INT(1, node2->in_list);
  TEST_ASSERT_EQUAL_INT(1, node3->in_list);

  clist_node_t *first = clist_first(head);
  clist_node_t *second = first->next;
  clist_node_t *third = second->next;

  TEST_ASSERT_EQUAL_PTR(first, node1);
  TEST_ASSERT_EQUAL_PTR(second, node2);
  TEST_ASSERT_EQUAL_PTR(third, node3);

  free(head);
  free(node1);
  free(node2);
  free(node3);
}

void test_clist_for_each_and_del(void) {
  clist_node_t *head = create_list();
  clist_node_t *node1 = create_node();
  clist_node_t *node2 = create_node();
  clist_node_t *node3 = create_node();

  clist_push_back(head, node1);
  clist_push_back(head, node2);
  clist_push_back(head, node3);

  int cnt = 0;
  clist_node_t *res = clist_for_each_and_del(head, callback_cnt, &cnt);
  TEST_ASSERT_NULL(res);
  TEST_ASSERT_EQUAL_INT(3, cnt);
  TEST_ASSERT_TRUE(clist_empty(head));
  TEST_ASSERT_EQUAL_INT(0, node1->in_list);
  TEST_ASSERT_EQUAL_INT(0, node2->in_list);
  TEST_ASSERT_EQUAL_INT(0, node3->in_list);

  clist_push_back(head, node1);
  clist_push_back(head, node2);
  clist_push_back(head, node3);

  cnt = 0;
  res = clist_for_each_and_del(head, callback_fail_on_first, &cnt);
  TEST_ASSERT_EQUAL_PTR(node1, res);
  TEST_ASSERT_EQUAL_INT(0, node1->in_list);
  TEST_ASSERT_EQUAL_INT(0, node2->in_list);
  TEST_ASSERT_EQUAL_INT(0, node3->in_list);
  TEST_ASSERT_TRUE(clist_empty(head));

  free(head);
  free(node1);
  free(node2);
  free(node3);
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_clist_init);
  RUN_TEST(test_clist_empty);
  RUN_TEST(test_clist_insert_before);
  RUN_TEST(test_clist_insert_after);
  RUN_TEST(test_clist_push_back);
  RUN_TEST(test_clist_is_in_list);
  RUN_TEST(test_clist_first);
  RUN_TEST(test_clist_last);
  RUN_TEST(test_clist_remove);
  RUN_TEST(test_clist_pop_front);
  RUN_TEST(test_clist_for_each);
  RUN_TEST(test_clist_find);
  RUN_TEST(test_clist_insert_sorted);
  RUN_TEST(test_clist_for_each_and_del);
  return UNITY_END();
}
