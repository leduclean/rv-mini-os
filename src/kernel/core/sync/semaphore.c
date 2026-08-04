#include "semaphore.h"
#include "clist.h"
#include "container.h"
#include "cpu.h"
#include "process.h"
#include "scheduler.h"
#include "minilib/stdint.h"
#include "waitqueue.h"

/** @brief Counting semaphore and the processes waiting on it. */
struct semaphore {
  int count;            ///< Number of available ressources.
  wait_queue_t waiting; ///< Processes blocked waiting for a ressource.
};

void sem_init(semaphore_t *sem, int val) {
  irq_flags_t flags = _irq_save();
  wq_init(&sem->waiting);
  sem->count = val;

  _irq_restore(flags);
}

int8_t sem_try_take(semaphore_t *sem) {
  // Semaphore op are atomics so we disable interupts
  irq_flags_t flags = _irq_save();
  if (sem->count <= 0) {
    _irq_restore(flags);
    return -1;
  }
  sem->count--;
  _irq_restore(flags);
  return 0;
}

void sem_take(semaphore_t *sem) {
  // Semaphore op are atomics so we disable interupts
  irq_flags_t flags = _irq_save();
  if (sem_try_take(sem) == -1) {
    scheduler_block_on(&sem->waiting);
  }
  _irq_restore(flags);
}

void sem_release(semaphore_t *sem) {
  // Semaphore op are atomics so we disable interupts
  irq_flags_t flags = _irq_save();

  // Release a ressource
  sem->count++;

  // Check in the waiting list if one
  // is waiting and wake him up if necessary.
  if (!wq_is_empty(&sem->waiting)) {
    clist_node_t *node = wq_pop_head(&sem->waiting);
    process_t *proc = container_of(node, process_t, wait_node);
    scheduler_ready_process(proc);
  }
  _irq_restore(flags);
}
