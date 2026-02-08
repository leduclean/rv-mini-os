#include "kernel/sync/semaphore.h"
#include "arch/riscv/cpu.h"
#include "kernel/process/process.h"
#include "kernel/sched/circ_queue.h"
#include "kernel/sched/scheduler.h"
#include "lib/stdint.h"
#include "lib/string.h"

struct semaphore {
  int count;
  wait_queue_t waiting;
};

/** Init a semaphore structure **/
void sem_init(semaphore_t *sem, int val) {
  irq_flags_t flags = irq_save();

  sem->waiting.head = NULL;
  sem->waiting.tail = NULL;
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
    process_t *proc = wq_pop_head(&sem->waiting);
    scheduler_ready_process(proc);
  }
  irq_restore(flags);
}
