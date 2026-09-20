#include <stdint.h>

#include <lib/clist.h>
#include <lib/container.h>

#include <asm/cpu.h>

#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/semaphore.h>
#include <kernel/spinlock.h>
#include <kernel/waitqueue.h>

void sem_init(semaphore_t *sem, int val)
{
	spinlock_init(&sem->lock);
	wq_init(&sem->wq);
	sem->count = val;
}

static int _sem_try_take_unlocked(semaphore_t *sem)
{
	if (sem->count <= 0) {
		return -1;
	}
	sem->count--;
	return 0;
}

int sem_try_take(semaphore_t *sem)
{
	int res;
	unsigned long flags = spinlock_lock_irq_save(&sem->lock);

	res = _sem_try_take_unlocked(sem);

	spinlock_unlock_irq_restore(&sem->lock, flags);
	return res;
}

void sem_wait(semaphore_t *sem)
{
	unsigned long flags = spinlock_lock_irq_save(&sem->lock);

	if (_sem_try_take_unlocked(sem) < 0) {
		scheduler_block_on_locked(&sem->wq, &sem->lock);
		irq_restore(flags);
		return;
	}

	spinlock_unlock_irq_restore(&sem->lock, flags);
}

void sem_post(semaphore_t *sem)
{
	unsigned long flags = spinlock_lock_irq_save(&sem->lock);

	if (!wq_is_empty(&sem->wq)) {
		// Directly give the ressource to a waiting thread
		// without posting thre ressource
		clist_node_t *node = wq_pop_head(&sem->wq);
		process_t *proc = container_of(node, process_t, wait_node);
		scheduler_ready_process(proc);
	} else {
		// Post a ressource
		sem->count++;
	}

	spinlock_unlock_irq_restore(&sem->lock, flags);
}
