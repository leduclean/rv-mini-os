#include "semaphore.h"
#include "clist.h"
#include "container.h"
#include "cpu.h"
#include "process.h"
#include "scheduler.h"
#include "minilib/stdint.h"
#include "waitqueue.h"

struct semaphore {
  int count;
  wait_queue_t waiting;
};

/** Init a semaphore structure **/
void sem_init(semaphore_t *sem, int val) {
  irq_flags_t flags = irq_save();
  wq_init(&sem->waiting);
  sem->count = val;

  irq_restore(flags);
}

/** Take a semaphore **/
int8_t sem_try_take(semaphore_t *sem) {
  // Semaphore op are atomics so we disable interupts
  irq_flags_t flags = irq_save();
  if (sem->count <= 0) {
    irq_restore(flags);
    return -1;
  }
  sem->count--;
  irq_restore(flags);
  return 0;
}

/** Take a semaphore **/
void sem_take(semaphore_t *sem) {
  // Semaphore op are atomics so we disable interupts
  irq_flags_t flags = irq_save();
  if (sem_try_take(sem) == -1) {
    scheduler_block_on(&sem->waiting);
  }
  irq_restore(flags);
}

/** Release a semaphore **/
void sem_release(semaphore_t *sem) {
  // Semaphore op are atomics so we disable interupts
  irq_flags_t flags = irq_save();

  // Release a ressource
  sem->count++;

  // Check in the waiting list if one
  // is waiting and wake him up if necessary.
  if (!wq_is_empty(&sem->waiting)) {
    clist_node_t *node = wq_pop_head(&sem->waiting);
    process_t *proc = container_of(node, process_t, wait_node);
    scheduler_ready_process(proc);
  }
  irq_restore(flags);
}
