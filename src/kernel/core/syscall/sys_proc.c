#include <stdint.h>

#include <asm/trap.h>

#include <kernel/process.h>
#include <kernel/scheduler.h>

uint8_t sys_get_pid()
{
	return process_active()->pid;
}

int8_t sys_fork()
{
	process_t *parent = process_active();
	process_t *child = process_spawn_child(parent);
	if (!child) {
		return -1;
	}

	return child->pid;
}

void sys_exit(int8_t code)
{
	scheduler_terminate(code);
}
