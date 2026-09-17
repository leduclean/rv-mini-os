#include <stdint.h>

#include <asm/cpu.h>
#include <asm/trap.h>

#include <kernel/cmd_registry.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>

#include "kernel/vpages.h"

uint8_t sys_getpid(void)
{
	return process_active()->pid;
}

int8_t sys_fork(void)
{
	process_t *parent = process_active();
	process_t *child = process_fork(parent);
	if (!child) {
		return -1;
	}

	return child->pid;
}

int sys_exec(const char *path, char *const argv[])
{
	//TODO: handle argv
	(void)argv;
	process_t *p = process_active();

	// Copy the virtual uaddres to a kernel buffer
	char kpath[MAXNAME];
	vpage_copyinstr(p->root_ptable, kpath, path, MAXNAME);

	// Look up in the process registry
	const cmd_desc_t *matched = cmd_lookup(kpath);
	if (!matched) {
		printf("Unknown path entered. Please refer to `help`.\n");
		return -1;
	}

	int res;
	res = process_exec(p, matched->cmd.prog.fn);
	if (res < 0) {
		return res;
	}

	return 0;
}

void sys_exit(uint8_t code)
{
	scheduler_terminate(code);
}
