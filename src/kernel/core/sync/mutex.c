#include "mutex.h"
#include "clist.h"
#include "container.h"
#include "cpu.h"
#include "process.h"
#include "scheduler.h"
#include "minilib/stdint.h"
#include "minilib/string.h"

void mutex_init(mutex_t *m) {
  // Atomic
  irq_flags_t flags = _irq_save();
  m->locked = 0;
  m->owner = NULL;
  wq_init(&m->wq);
  _irq_restore(flags);
}

uint8_t mutex_is_lock(const mutex_t *m) { return m->locked; }

process_t *mutex_owner(const mutex_t *m) { return m->owner; }

int8_t mutex_trylock(mutex_t *m) {
  // Atomic
  irq_flags_t flags = _irq_save();
  if (m->locked) {
    // Already locked
    _irq_restore(flags);
    return -1;
  }

  // If not locked take the onership.
  m->locked = 1;
  m->owner = get_active();
  _irq_restore(flags);
  return 0;
}

void mutex_lock(mutex_t *m) {
  irq_flags_t flags = _irq_save();

  if (mutex_trylock(m) == -1) {
    scheduler_block_on(&m->wq);
  }

  _irq_restore(flags);
}

void mutex_unlock(mutex_t *m) {
  // Atomic
  irq_flags_t flags = _irq_save();
  process_t *current = get_active();
  if ((!m->locked) || (m->owner != current)) {
    _irq_restore(flags);
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
  _irq_restore(flags);
}
