#include "clist.h"
#include "process.h"
#include "unity/unity.h"
#include "unity/unity_fixture.h"

void test_idle() {}
void proc_test_code() {}

TEST_GROUP(Process);

TEST_SETUP(Process) {}
TEST_TEAR_DOWN(Process) {}

TEST(Process, Wake) {
  clist_node_t mock_block_queue;
  clist_node_t mock_sleeping_queue;
  clist_init_node(&mock_block_queue);
  clist_init_node(&mock_sleeping_queue);
  process_t p1 = {.state = SLEEPING, .pid = 1};
  process_t p2 = {.state = BLOCKED, .pid = 2};
  process_t p3 = {.state = READY, .pid = 3};

  clist_init_node(&p1.sleep_node);
  clist_init_node(&p2.wait_node);
  clist_init_node(&p3.sleep_node); // ou ready_node si besoin

  clist_push_back(&p1.sleep_node, &mock_sleeping_queue);
  clist_push_back(&p2.wait_node, &mock_block_queue);

  process_wake(&p1);
  process_wake(&p2);
  process_wake(&p3);

  TEST_ASSERT_EQUAL_INT(p1.state, RUNNING);
  TEST_ASSERT_EQUAL_INT(p2.state, RUNNING);
  // Remain unchanges on non blocked or non sleeping
  TEST_ASSERT_EQUAL_INT(p3.state, READY);

  TEST_ASSERT_EQUAL_INT(clist_is_in_list(&p1.sleep_node), 0);
  TEST_ASSERT_EQUAL_INT(clist_is_in_list(&p2.wait_node), 0);
}

TEST_GROUP_RUNNER(Process) { RUN_TEST_CASE(Process, Wake); }
