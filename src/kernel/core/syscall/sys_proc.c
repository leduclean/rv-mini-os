#include <stdint.h>

#include <lib/err.h>

#include <arch/cpu.h>
#include <arch/trap.h>
#include <kernel/task.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>

extern char _user_elf_start[];

uint8_t sys_getpid(void)
{
	return task_active()->pid;
}

int8_t sys_fork(void)
{
	task_t *parent = task_active();
	task_t *child = task_fork(parent);
	if (IS_ERR(child)) {
		return PTR_ERR(child);
	}

	return child->pid;
}

int8_t sys_clone(void *entry, void *args, void *stack, unsigned long flags)
{
	task_t *parent = task_active();
	task_t *child = task_clone(parent, entry, args, stack, flags);
	if (IS_ERR(child)) {
		return PTR_ERR(child);
	}


	return child->pid;
}

int sys_exec(const char *path, char *const argv[])
{
	//TODO: handle argv

	(void)argv;
	(void)path;
	task_t *p = task_active();

	// Copy the virtual uaddres to a kernel buffer
	// char kpath[MAXNAME];
	// vpage_copyinstr(p->root_ptable, kpath, path, MAXNAME);
	//    // Read only the content because elf file is copied before being executed
	// struct file *f = file_open(kpath, O_R);
	// if (!f) {
	// 	return -1;
	// }
	//
	//TODO: stub before fs mkfs
	return task_exec(p, (uintptr_t)_user_elf_start, argv);
}

void sys_exit(uint8_t code)
{
	scheduler_exit(code);
}

void sys_exit_group(uint8_t code)
{
	scheduler_exit_group(code);
};
