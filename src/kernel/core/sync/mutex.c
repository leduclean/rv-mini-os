#include <stdint.h>

#include <lib/clist.h>
#include <lib/container.h>
#include <lib/string.h>

#include <asm/cpu.h>

#include <kernel/mutex.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/spinlock.h>

void mutex_init(mutex_t *m)
{
	spinlock_init(&m->lock);
	wq_init(&m->wq);
	m->locked = 0;
	m->owner = NULL;
}

uint8_t mutex_is_lock(const mutex_t *m)
{
	return m->locked;
}

process_t *mutex_owner(const mutex_t *m)
{
	return m->owner;
}

static int _mutex_trylock_unlocked(mutex_t *m)
{
	if (m->locked) {
		// Already locked
		return -1;
	}

	// If not locked take the ownership.
	m->locked = 1;
	m->owner = process_active();
	return 0;
}

int mutex_trylock(mutex_t *m)
{
	int res;
	spinlock_lock(&m->lock);

	res = _mutex_trylock_unlocked(m);

	spinlock_unlock(&m->lock);
	return res;
}

void mutex_lock(mutex_t *m)
{
	spinlock_lock(&m->lock);

	if (_mutex_trylock_unlocked(m) == -1) {
		scheduler_block_on_locked(&m->wq, &m->lock);
		return;
	}

	spinlock_unlock(&m->lock);
}

void mutex_unlock(mutex_t *m)
{
	spinlock_lock(&m->lock);

	process_t *current = process_active();
	if ((!m->locked) || (m->owner != current)) {
		goto out;
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

out:
	spinlock_unlock(&m->lock);
}
