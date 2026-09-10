#include "process.h"
#include "scheduler.h"
#include "minilib/stdint.h"
#include "trap.h"

uint8_t sys_get_pid()
{
	return get_active_pid();
}

int8_t sys_fork()
{
	process_t *parent = get_active();
	process_t *child = spawn_child(parent);
	if (!child) {
		return -1;
	}

	return child->pid;
}

void sys_exit(int8_t code)
{
	scheduler_terminate(code);
}
