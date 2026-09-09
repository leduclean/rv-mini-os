#include "sync.h"
#include "clist.h"
#include "container.h"
#include "cpu.h"
#include "process.h"
#include "scheduler.h"
#include "minilib/stddef.h"
#include "minilib/stdint.h"

uint8_t wait()
{
	irq_flags_t flags = irq_save();

	process_t *parent = get_active();
	//TODO: Should return an error this process has no child
	wait_queue_t *zombies = get_zombies(parent);
	while (wq_is_empty(zombies)) {
		scheduler_block_on(get_wait_child_queue(parent));
	}
	// Remove the first element of the zombie queue
	clist_node_t *node = wq_pop_head(zombies);
	process_t *to_reap = container_of(node, process_t, wait_node);
	uint8_t pid = get_pid(to_reap);
	process_reap(to_reap);

	irq_restore(flags);
	return pid;
}

uint8_t wait_pid(int8_t pid)
{
	irq_flags_t flags = irq_save();

	process_t *parent = get_active();
	wait_queue_t *zombies = get_zombies(parent);
	process_t *to_reap = NULL;
	while ((to_reap = wq_remove_by_pid(zombies, pid)) == NULL) {
		// Remove the element by pid
		scheduler_block_on(get_wait_child_queue(parent));
	}
	process_reap(to_reap);

	irq_restore(flags);
	return pid;
}
