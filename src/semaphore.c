#include "semaphore.h"
#include "circ_queue.h"
#include "cpu.h"
#include "process.h"
#include "scheduler.h"
#include <stdint.h>
#include <string.h>

struct semaphore {
  int count;
  wait_queue_t waiting;
};

/** Init a semaphore structure **/
void sem_init(semaphore_t *sem, int val) {
  // Semaphore op are atomics so we disable interupts
  disable_it();
  sem->waiting.head = NULL;
  sem->waiting.tail = NULL;
  sem->count = val;
  enable_it();
}

/** Take a semaphore **/
int8_t sem_try_take(semaphore_t *sem) {
  // Semaphore op are atomics so we disable interupts
  disable_it();
  if (sem->count <= 0) {
    enable_it();
    return -1;
  }
  sem->count--;
  enable_it();
  return 0;
}

/** Take a semaphore **/
void sem_take(semaphore_t *sem) {
  // Semaphore op are atomics so we disable interupts
  disable_it();
  if (sem_try_take(sem) == -1) {
    scheduler_block_on(&sem->waiting);
  }
  enable_it();
}

/** Release a semaphore **/
void sem_release(semaphore_t *sem) {
  // Semaphore op are atomics so we disable interupts
  disable_it();

  // Release a ressource
  sem->count++;

  // Check in the waiting list if one
  // is waiting and wake him up if necessary.
  if (!wq_is_empty(&sem->waiting)) {
    process_t *proc = wq_pop_head(&sem->waiting);
    scheduler_ready_process(proc);
  }
  enable_it();
}
