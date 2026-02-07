#include "mutex.h"
#include "circ_queue.h"
#include "cpu.h"
#include "process.h"
#include "scheduler.h"
#include "string.h"
#include <stdint.h>

struct mutex {
  uint8_t locked;
  process_t *owner;
  wait_queue_t wq;
};

/** Mutex Init primitive **/
void mutex_init(mutex_t *m) {
  // Atomic
  disable_it();
  m->locked = 0;
  m->owner = NULL;
  wq_init(&m->wq);
  enable_it();
}

/** Give the lock state of the mutex **/
uint8_t mutex_is_lock(mutex_t *m) { return m->locked; }

/** Give the owner of the mutex **/
process_t *mutex_owner(mutex_t *m) { return m->owner; }

/** TryLock primitive to try to take the ownership of a mutex
 * return 0 if taken else -1. Do not block if not taken **/
int8_t mutex_trylock(mutex_t *m) {
  // Atomic
  disable_it();
  if (m->locked) {
    // Already locked
    enable_it();
    return -1;
  }

  // If not locked take the ownership.
  m->locked = 1;
  m->owner = get_active();
  enable_it();
  return 0;
}

/** Lock primitive to take ownership of a mutex **/
void mutex_lock(mutex_t *m) {
  // Atomic
  disable_it();
  if (mutex_trylock(m) == -1) {
    scheduler_block_on(&m->wq);
  }
  enable_it();
}

/** Unlock primitive to release ownership of a mutex **/
void mutex_unlock(mutex_t *m) {
  // Atomic
  disable_it();
  process_t *current = get_active();
  if ((!m->locked) || (m->owner != current)) {
    enable_it();
    return;
  }

  if (!wq_is_empty(&m->wq)) {
    // Wake first waiting and give him the ownership.
    process_t *proc = wq_pop_head(&m->wq);
    m->owner = proc;
    scheduler_ready_process(proc);
  } else {
    // Unlock and reset owner.
    m->locked = 0;
    m->owner = NULL;
  }
  enable_it();
}
