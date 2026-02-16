#include "mutex.h"
#include "clist.h"
#include "container.h"
#include "cpu.h"
#include "process.h"
#include "scheduler.h"
#include "minilib/stdint.h"
#include "minilib/string.h"

struct mutex {
  uint8_t locked;
  process_t *owner;
  wait_queue_t wq;
};

/** Mutex Init primitive **/
void mutex_init(mutex_t *m) {
  // Atomic
  irq_flags_t flags = irq_save();
  m->locked = 0;
  m->owner = NULL;
  wq_init(&m->wq);
  irq_restore(flags);
}

/** Give the lock state of the mutex **/
uint8_t mutex_is_lock(mutex_t *m) { return m->locked; }

/** Give the owner of the mutex **/
process_t *mutex_owner(mutex_t *m) { return m->owner; }

/** TryLock primitive to try to take the ownership of a mutex
 * return 0 if taken else -1. Do not block if not taken **/
int8_t mutex_trylock(mutex_t *m) {
  // Atomic
  irq_flags_t flags = irq_save();
  if (m->locked) {
    // Already locked
    irq_restore(flags);
    return -1;
  }

  // If not locked take the onership.
  m->locked = 1;
  m->owner = get_active();
  irq_restore(flags);
  return 0;
}

/** Lock primitive to take ownership of a mutex **/
void mutex_lock(mutex_t *m) {
  irq_flags_t flags = irq_save();

  if (mutex_trylock(m) == -1) {
    scheduler_block_on(&m->wq);
  }

  irq_restore(flags);
}

/** Unlock primitive to release ownership of a mutex **/
void mutex_unlock(mutex_t *m) {
  // Atomic
  irq_flags_t flags = irq_save();
  process_t *current = get_active();
  if ((!m->locked) || (m->owner != current)) {
    irq_restore(flags);
    return;
  }

  if (!wq_is_empty(&m->wq)) {
    // Wake first waiting and give him the ownership.
    clist_node_t *node = wq_pop_head(&m->wq);
    process_t *proc = container_of(node, process_t, wait_node);
    m->owner = proc;
    scheduler_ready_process(proc);
  } else {
    // Unlock and reset owner.
    m->locked = 0;
    m->owner = NULL;
  }
  irq_restore(flags);
}
