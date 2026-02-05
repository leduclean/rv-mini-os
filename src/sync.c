#include "sync.h"
#include "circ_queue.h"
#include "process.h"
#include "scheduler.h"
#include <stddef.h>
#include <stdint.h>

/** Wait sync primitive to wait for a child terminaison **/
uint8_t wait() {
  process_t *parent = get_active();
  circ_queue_t *zombies = get_zombies(parent);
  while (is_empty(zombies)) {
    scheduler_block_on(get_wait_child_queue(parent));
  }
  // Remove the first element of the zombie queue
  process_t *reaped = pop(zombies);
  set_state(reaped, TERMINATED);
  return get_pid(reaped);
}

/** Wait Pid sync primitive to wait for a specific child terminaison **/
uint8_t wait_pid(int8_t pid) {
  process_t *parent = get_active();
  circ_queue_t *zombies = get_zombies(parent);
  process_t *reaped = NULL;
  while ((reaped = circ_remove_by_pid(zombies, pid)) == NULL) {
    // Remove the element by pid
    scheduler_block_on(get_wait_child_queue(parent));
  }
  set_state(reaped, TERMINATED);
  return get_pid(reaped);
}
