/**
 * @file
 * @brief Parent side synchronisation syscalls.
 */

#include <clist.h>
#include <container.h>
#include "cpu.h"
#include "process.h"
#include "scheduler.h"
#include <stddef.h>
#include <stdint.h>

int8_t sys_wait()
{
	irq_flags_t flags = irq_save();

	process_t *parent = process_active();
	//TODO: Should return an error this process has no child
	wait_queue_t *zombies = &parent->zombies;
	while (wq_is_empty(zombies)) {
		scheduler_block_on(&parent->child_wq);
	}
	// Remove the first element of the zombie queue
	clist_node_t *node = wq_pop_head(zombies);
	process_t *to_reap = container_of(node, process_t, wait_node);
	uint8_t pid = to_reap->pid;
	process_reap(to_reap);

	irq_restore(flags);
	return pid;
}

int8_t sys_wait_pid(int8_t pid)
{
	irq_flags_t flags = irq_save();

	process_t *parent = process_active();
	wait_queue_t *zombies = &parent->zombies;
	process_t *to_reap = NULL;
	while ((to_reap = wq_remove_by_pid(zombies, pid)) == NULL) {
		// Remove the element by pid
		scheduler_block_on(&parent->child_wq);
	}
	process_reap(to_reap);

	irq_restore(flags);
	return pid;
}
