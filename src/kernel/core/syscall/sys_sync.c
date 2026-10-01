/**
 * @file
 * @brief Parent side synchronisation syscalls.
 */

#include <stddef.h>
#include <stdint.h>

#include <lib/clist.h>
#include <lib/container.h>

#include <asm/cpu.h>

#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>

#include "asm/trap.h"

int8_t sys_wait(void)
{
	irq_flags_t flags = irq_save();

	process_t *parent = process_active();
	//TODO: Should return an error this process has no child
	clist_node_t *zombies = &parent->zombies;
	while (clist_empty(zombies)) {
		scheduler_block_on(&parent->zombie_wq);
	}
	// Remove the first element of the zombie queue
	clist_node_t *node = clist_pop_front(zombies);
	process_t *to_reap = container_of(node, process_t, wait_node);
	uint8_t pid = to_reap->pid;
	process_reap(to_reap);

	irq_restore(flags);
	return pid;
}

static inline int _find_pid_cb(clist_node_t *node, void *args)
{
	int8_t pid = *(int8_t *)args;
	process_t *p = container_of(node, process_t, wait_node);
	return pid == p->pid;
}

int8_t sys_waitpid(int8_t pid)
{
	irq_flags_t flags = irq_save();

	process_t *parent = process_active();
	clist_node_t *zombies = &parent->zombies;
	clist_node_t *found_node = NULL;

	while ((found_node = clist_find(zombies, _find_pid_cb, &pid)) == NULL) {
		// Wait on its wait queue
		scheduler_block_on(&parent->zombie_wq);
	}
	clist_remove(found_node);

	process_t *to_reap = container_of(found_node, process_t, wait_node);
	process_reap(to_reap);

	irq_restore(flags);
	return pid;
}
