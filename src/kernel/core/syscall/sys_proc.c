#include <stdint.h>

#include <lib/err.h>

#include <arch/cpu.h>
#include <arch/trap.h>
#include <kernel/file.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>
#include <kernel/task.h>

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
	int res;
	task_t *p = task_active();

	char kpath[MAXNAME];
	res = vpage_copyinstr(p->root_ptable, kpath, path, MAXNAME);
	if (res < 0) {
		return res;
	}

	struct file *user_elf = file_open(kpath, O_R);
	if (IS_ERR(user_elf)) {
		return PTR_ERR(user_elf);
	};
	res = task_exec(p, user_elf, argv);
	file_put(user_elf);
	return res;
}

void sys_exit(uint8_t code)
{
	scheduler_exit(code);
}

void sys_exit_group(uint8_t code)
{
	scheduler_exit_group(code);
};
