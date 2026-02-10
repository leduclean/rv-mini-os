#include "kernel/sync/sync.h"
#include "arch/riscv/cpu.h"
#include "kernel/process/process.h"
#include "kernel/sched/circ_queue.h"
#include "kernel/sched/scheduler.h"
#include "lib/clist.h"
#include "lib/container.h"
#include "lib/stddef.h"
#include "lib/stdint.h"

/** Wait sync primitive to wait for a child terminaison **/
uint8_t wait() {
  irq_flags_t flags = irq_save();

  process_t *parent = get_active();
  wait_queue_t *zombies = get_zombies(parent);
  while (wq_is_empty(zombies)) {
    scheduler_block_on(get_wait_child_queue(parent));
  }
  // Remove the first element of the zombie queue
  clist_node_t *node = wq_pop_head(zombies);
  process_t *reaped = container_of(node, process_t, wait_node);
  set_state(reaped, TERMINATED);

  irq_restore(flags);
  return get_pid(reaped);
}

/** Wait Pid sync primitive to wait for a specific child terminaison **/
uint8_t wait_pid(int8_t pid) {
  irq_flags_t flags = irq_save();

  process_t *parent = get_active();
  wait_queue_t *zombies = get_zombies(parent);
  process_t *reaped = NULL;
  while ((reaped = wq_remove_by_pid(zombies, pid)) == NULL) {
    // Remove the element by pid
    scheduler_block_on(get_wait_child_queue(parent));
  }
  set_state(reaped, TERMINATED);

  irq_restore(flags);
  return get_pid(reaped);
}
