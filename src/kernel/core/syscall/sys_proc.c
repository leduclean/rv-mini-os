#include <stdint.h>

#include <asm/cpu.h>
#include <asm/trap.h>

#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>

#include "kernel/file.h"
#include "kernel/inode.h"
#include "kernel/vpages.h"
#include "user/kinterface.h"

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

int8_t sys_clone(void *entry, void *args, void *stack, unsigned long flags)
{
	process_t *parent = process_active();
	process_t *child = process_clone(parent, entry, args, stack, flags);
	if (!child) {
		return -1;
	}

	return child->pid;
}

int sys_exec(const char *path, char *const argv[])
{
	//TODO: handle argv

	(void)argv;
	(void)path;
	// process_t *p = process_active();
	//
	// // Copy the virtual uaddres to a kernel buffer
	// char kpath[MAXNAME];
	// vpage_copyinstr(p->root_ptable, kpath, path, MAXNAME);
	//    // Read only the content because elf file is copied before being executed
	// struct file *f = file_open(kpath, O_R);
	// if (!f) {
	// 	return -1;
	// }
	//
	//    char file

	//TODO: stub befor fs link with elf
	return 0;
}

void sys_exit(uint8_t code)
{
	scheduler_exit(code);
}

void sys_exit_group(uint8_t code)
{
	scheduler_exit_group(code);
};
