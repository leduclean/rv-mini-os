#include <stddef.h>
#include <stdint.h>

#include <lib/clist.h>
#include <lib/string.h>
#include <lib/tinyalloc.h>

#include <arch/asm_defs.h>
#include <arch/cpu.h>
#include <arch/trap.h>
#include <kernel/elf.h>
#include <kernel/file.h>
#include <kernel/mmap.h>
#include <kernel/pages.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>
#include <kernel/time.h>
#include <kernel/vpages.h>
#include <kernel/waitqueue.h>
#include <user/kinterface.h>

#include "arch/abi.h"
#include "lib/bit.h"

#define MAX_PROC 32

/** @brief Process table. */
typedef struct {
	uint8_t next_pid; ///< Pid given to the next spawned process.
	uint8_t active_process; ///< Number of living processes, not TERMINATED.
	clist_node_t head; ///< Head sentinel of the living processes clist.
} ptable_t;

/** @brief The one process table. */
static ptable_t proc_table;

/** @brief Currently running process. */
static process_t *active = NULL;

/** @brief Init the process table. */
static void _init_proc_table(void)
{
	clist_init_node(&proc_table.head);
	proc_table.next_pid = 0;
	proc_table.active_process = 0;
}

/**
 * @brief Remove a process from all the blocking queues it could wait on.
 *
 * @note This function is **not atomic**. The caller MUST ensure atomicity.
 *
 * @param proc Process to clear.
 */
static void _remove_from_blocking_queues(process_t *proc)
{
	if (clist_is_in_list(&proc->sleep_node))
		clist_remove(&proc->sleep_node);
	if (clist_is_in_list(&proc->wait_node)) {
		clist_remove(&proc->wait_node);
	}
}

/**
 * @brief Remove a process from all the scheduling queues it could be in.
 *
 * @note This function is **not atomic**. The caller MUST ensure atomicity.
 *
 * @param proc Process to remove from the queues.
 */
static inline void _remove_from_sched_queues(process_t *proc)
{
	if (clist_is_in_list(&proc->ready_node))
		clist_remove(&proc->ready_node);
	_remove_from_blocking_queues(proc);
}

/**
 * @brief Remove a process from all the queues it could be in.
 *
 * @note This function is **not atomic**. The caller MUST ensure atomicity.
 *
 * @param proc Process to remove from the queues.
 */
static inline void _remove_from_all_queues(process_t *proc)
{
	if (clist_is_in_list(&proc->proc_node))
		clist_remove(&proc->proc_node);
	_remove_from_sched_queues(proc);
}

/**
 * @brief Reset all the nodes and the wait queues of a process.
 *
 * @param proc Process to reset.
 */
static inline void _init_process_queues(process_t *proc)
{
	clist_init_node(&proc->proc_node);
	clist_init_node(&proc->ready_node);
	clist_init_node(&proc->wait_node);
	clist_init_node(&proc->sleep_node);
	clist_init_node(&proc->zombies);
	wq_init(&proc->zombie_wq);
}

/**
 * @brief Alloc a process squeleton ie without activating it or setting context.
 * 
 * @param name Name of the proc. 
 * @param prior Priority of the proc.
 * @param user User proc flag.
 * @param parent Parent of the proc.
 * @return NULL on failure, else the new allocated proc structure.
 */
static inline process_t *_alloc_squeletton(const char *name, priority prior,
					   bool user, process_t *parent)
{
	if (proc_table.active_process >= MAX_PROC) {
		return NULL; // Error already max processus launched
	}

	process_t *proc = calloc(1, sizeof(process_t));
	if (!proc) {
		return NULL;
	}

	_init_process_queues(proc);

	strncpy(proc->name, name, sizeof(proc->name) - 1);
	proc->user = user;
	proc->parent = parent;
	proc->priority = prior;
	proc->pid = proc_table.next_pid++;
	proc->tgid = proc->pid;
	proc->state = NEW;

	return proc;
}

/**
 * @brief Activate with the scheduler a new process.
 *
 * @param p A pointer to the process.
 */
static void _activate_process(process_t *p)
{
	p->state = READY;
	clist_push_back(&proc_table.head, &p->proc_node);
	proc_table.active_process++;
	scheduler_admit(p);
	return;
}

/**
 * @brief Kernel privilege proessus entry point.
 */
static void _kproc_launcher(void)
{
	//NOTE: We force this because there is no guarantee,
	// after a kprocess spawn to have irq enable.
	// (ctx switch does not preserve the sstatus)
	irq_enable_s();
	process_t *p = process_active();
	p->code();
	scheduler_exit_group(0);
}

/** @brief Create the idle process and make it active. */
static int _init_idle(void)
{
	process_t *p = process_kthread(process_idle, "idle", IDLE);
	if (!p) {
		return -1;
	}
	active = p;
	active->state = RUNNING;
	return 0;
}

process_t *process_active(void)
{
	return active;
}

void process_switch_active(process_t *next)
{
	if (active->state == RUNNING)
		active->state = READY;

	next->state = RUNNING;
	active = next;
}

const clist_node_t *process_table_clist(void)
{
	return &proc_table.head;
}

uint8_t priority_higher(priority prior, priority other)
{
	return prior < other;
}

void process_sleep(process_t *proc, uint32_t delay)
{
	proc->state = SLEEPING;
	proc->wake_up_time = delay + time_seconds();
}

void process_block(process_t *proc)
{
	proc->state = BLOCKED;
}

void process_wake(process_t *proc)
{
	irq_flags_t state = irq_save();
	if ((proc->state == SLEEPING) || (proc->state == BLOCKED)) {
		// Remove it from sleeping and blocked queue if remaining
		_remove_from_blocking_queues(proc);
		proc->state = RUNNING;
	}
	irq_restore(state);
}

static void release_fds(process_t *p)
{
	for (int i = 0; i < NOFILE; i++) {
		struct file *f = p->fdtable[i];
		if (f) {
			file_put(f);
		}
	}
}

void process_zombify(process_t *proc, int exit_code)
{
	irq_flags_t state = irq_save();

	process_t *parent = proc->parent;
	if (parent == NULL) {
		panic("Zombify an orphan process is not permitted");
	}

	proc->state = ZOMBIE;
	proc->exit_code = exit_code;

	_remove_from_sched_queues(proc);
	release_fds(proc);
	clist_push_back(&parent->zombies, &proc->wait_node);

	if (parent->state == BLOCKED) {
		// Parent is waiting so we wake him up
		// to check if he can stop wait.
		scheduler_wake_waiting_queue(&parent->zombie_wq);
	}

	irq_restore(state);
}

void process_destroy(process_t *proc)
{
	irq_flags_t state = irq_save();
	if (process_active() == proc) {
		panic("Trying to destroy the current process led to segfault. Use zombify instead");
	}

	proc->state = TERMINATED;
	_remove_from_all_queues(proc);

	pte_t *root = proc->root_ptable;
	if (proc->user && root) {
		vpage_unmmap(proc->root_ptable, (void *)proc->tframe_va);
		if (page_get_ref_count(root) == 1) {
			vpage_tree_free(proc->root_ptable);
		} else {
			page_put(root);
		}
	}

	free(proc);
	proc_table.active_process--;

	irq_restore(state);
}

static void copy_fds(process_t *child, const process_t *parent, bool clone)
{
	if (!parent) {
		return;
	}

	for (int i = 0; i < NOFILE; i++) {
		struct file *f = parent->fdtable[i];
		if (f) {
			if (clone) {
				child->fdtable[i] = file_clone(f);
			} else {
				child->fdtable[i] = file_get(f);
			}
		}
	}
}

static int load_elf_image(process_t *p, const uintptr_t elf)
{
	int res = elf_verify_file_format(elf);
	if (res < 0) {
		return -1;
	}

	if (mmap_uimage(p) != 0) {
		return -1;
	}

	res = elf_map(p->root_ptable, elf);
	if (res < 0) {
		vpage_tree_free(p->root_ptable);
		return -1;
	}

	return 0;
}

static int get_argc(pte_t *old_ptable, char *const argv[], char *argv_k[])
{
	int res;
	int argc = 0;

	if (!argv) {
		return argc;
	}

	while (argc < EXEC_MAX_ARGS) {
		res = vpage_copyin(old_ptable, &argv_k[argc], &argv[argc],
				   sizeof(*argv_k));
		if (res < 0) {
			return res;
		}

		if (!argv_k[argc]) {
			return argc;
		}

		argc++;
	}

	return -1;
}

static int transfer_argv(pte_t *old_ptable, pte_t *new_ptable,
			 char *const argv[], uintptr_t *out_sp)
{
	ssize_t len;
	uintptr_t sp = USTACK;
	char *argv_k[EXEC_MAX_ARGS];
	int argc = get_argc(old_ptable, argv, argv_k);
	if (argc < 0) {
		return -1;
	}

	char *argv_tmp[EXEC_MAX_ARGS];
	argv_tmp[argc] = NULL;

	char arg[MAXNAME];
	for (int i = argc - 1; i >= 0; i--) {
		len = vpage_copyinstr(old_ptable, arg, argv_k[i], MAXNAME);
		if (len < 0) {
			return len;
		}

		sp = ALIGN_DOWN(sp - len, STACK_ALIGN);

		len = vpage_copyoutstr(new_ptable, (char *)sp, arg, len);
		if (len < 0) {
			return len;
		}
		argv_tmp[i] = (char *)sp;
	}

	/* Align sp to copy argv */
	size_t argv_size = (argc + 1) * sizeof(argv_tmp[0]);
	sp = ALIGN_DOWN(sp - argv_size, STACK_ALIGN);

	/* Copy back argc */
	int res = vpage_copyout(new_ptable, (void *)sp, argv_tmp, argv_size);
	if (res < 0) {
		return res;
	}

	/* Then copy the new argv table pointers */
	sp = ALIGN_DOWN(sp - sizeof(argc), STACK_ALIGN);
	res = vpage_copyout(new_ptable, (void *)sp, &argc, sizeof(argc));
	if (res < 0) {
		return res;
	}

	*out_sp = sp;
	return 0;
}

process_t *process_start_userland(const uintptr_t user_elf, const char *name,
				  priority prior)
{
	irq_flags_t state = irq_save();
	process_t *p = _alloc_squeletton(name, prior, true, process_active());
	if (!p) {
		goto err_restore_irq;
	}

	p->ctx.sp = (uint64_t)&p->kstack[KSTACK_SIZE];
	p->ctx.ra = (uintptr_t)trap_return_to_user;

	int res = load_elf_image(p, user_elf);
	if (res < 0) {
		goto err_free_proc;
	}

	uintptr_t sp;
	res = transfer_argv(NULL, p->root_ptable, NULL, &sp);
	if (res < 0) {
		goto err_free_proc;
	}

	tframe_init(p->tframe_pa, p->root_ptable,
		    (unsigned long)&p->kstack[KSTACK_SIZE]);
	tframe_start(p->tframe_pa, (void *)elf_get_entry(user_elf), (void *)sp);

	copy_fds(p, process_active(), false);
	_activate_process(p);
	irq_restore(state);
	return p;

err_free_proc:
	free(p);
	p = NULL;
err_restore_irq:
	irq_restore(state);
	return p;
}

process_t *process_kthread(void code(void), const char *name, priority prior)
{
	irq_flags_t state = irq_save();
	process_t *p = _alloc_squeletton(name, prior, false, process_active());
	if (!p) {
		goto err_restore_irq;
	}

	p->code = code;
	if (p->pid == 0) {
		p->ctx.ra = (uintptr_t)code;
	} else {
		p->ctx.sp = (uint64_t)&p->kstack[KSTACK_SIZE];
		p->ctx.ra = (uintptr_t)_kproc_launcher;
	}

	copy_fds(p, process_active(), false);
	_activate_process(p);
	irq_restore(state);
	return p;

err_restore_irq:
	irq_restore(state);
	return p;
}

process_t *process_clone(process_t *parent, void *entry, void *args,
			 void *stack, unsigned long flags)
{
	if (!parent->user) {
		panic("Only fork for user process is supported ");
	}

	irq_flags_t state = irq_save();
	process_t *child = _alloc_squeletton(parent->name, parent->priority,
					     parent->user, parent);
	if (!child) {
		goto err_restore_irq;
	}

	tframe_t *t = page_alloc();
	if (!t) {
		goto err_free_proc;
	}
	child->tframe_pa = t;

	if (flags & CLONE_VM) {
		// Add a reference to this page.
		child->root_ptable = page_get((void *)parent->root_ptable);
		child->tgid = parent->tgid;

		if (!stack || !entry) {
			goto err_free_map;
		}
		tframe_start(t, entry, stack);
		tframe_set_arg(t, 0, (unsigned long)args);
		child->tframe_va = THREAD_TRAPFRAME(child->pid);
	} else {
		pte_t *root = page_alloc();
		if (!root) {
			goto err_free_tframe;
		}
		child->root_ptable = root;
		child->tgid = child->pid;

		// CoW all the tree.
		if (vpage_tree_copy(root, parent->root_ptable) < 0) {
			goto err_free_map;
		}

		// Copy the trap frame of the parent to continue at the same state.
		memcpy(t, parent->tframe_pa, sizeof(*child->tframe_pa));
		// Child returns 0 from the original state. (ie fork() return 0 in the child)
		tframe_set_return_val(t, 0);
		child->tframe_va = TRAPFRAME;
	}

	// Map the trap frame into the table
	int res = vpage_map(child->root_ptable, (void *)child->tframe_va, t,
			    PTE_R | PTE_W);
	if (res < 0) {
		// t failed to get mapped so no in the tree yet
		goto err_free_map;
	}

	tframe_init(t, child->root_ptable,
		    (unsigned long)&child->kstack[KSTACK_SIZE]);

	// On ctx switch on fork, we want this state
	child->ctx.ra = (unsigned long)trap_return_to_user;
	child->ctx.sp = (unsigned long)&child->kstack[KSTACK_SIZE];

	copy_fds(child, parent, flags & CLONE_FILES);

	_activate_process(child);
	irq_restore(state);
	return child;

err_free_map:
	if (flags & CLONE_VM) {
		page_put(child->root_ptable);
	} else {
		vpage_tree_free(child->root_ptable);
	}
err_free_tframe:
	page_put(child->tframe_pa);
err_free_proc:
	free(child);
	child = NULL;
err_restore_irq:
	irq_restore(state);
	return child;
}

process_t *process_fork(process_t *parent)
{
	return process_clone(parent, NULL, NULL, NULL, 0);
}

int process_exec(process_t *p, const uintptr_t elf, char *const argv[])
{
	int res;

	if (!p->user) {
		panic("Process exec on non user is not supported");
	}

	// Save the state
	irq_flags_t state = irq_save();
	pte_t *old_ptable = p->root_ptable;
	tframe_t *old_tframe = p->tframe_pa;

	res = load_elf_image(p, elf);
	if (res < 0) {
		goto err_restore_old_state;
	}

	uintptr_t sp;
	res = transfer_argv(old_ptable, p->root_ptable, argv, &sp);
	if (res < 0) {
		goto err_restore_old_state;
	}

	tframe_init(p->tframe_pa, p->root_ptable,
		    (unsigned long)&p->kstack[KSTACK_SIZE]);
	tframe_start(p->tframe_pa, (void *)elf_get_entry(elf), (void *)sp);

	// Once all has worked free all the old ressources.
	vpage_tree_free(old_ptable);
	irq_restore(state);

	return 0;

err_restore_old_state:
	p->root_ptable = old_ptable;
	p->tframe_pa = old_tframe;
	irq_restore(state);
	return res;
}

int process_give_fd(process_t *p, struct file *f)
{
	for (int fd = 0; fd < NOFILE; fd++) {
		if (p->fdtable[fd] == NULL) {
			p->fdtable[fd] = f;
			return fd;
		}
	}

	return -1;
}

struct file *process_get_fd(const process_t *p, int fd)
{
	if (fd >= NOFILE || fd < 0) {
		return NULL;
	}
	return p->fdtable[fd];
}

int process_release_fd(process_t *p, int fd)
{
	struct file *f = p->fdtable[fd];
	if (!f) {
		return -1;
	}
	p->fdtable[fd] = NULL;
	file_put(f);

	return 0;
}

int process_map_console(process_t *p)
{
	struct file *f;
	int res = fs_mknod("/dev/console", FILE_DEV_CONSOLE);
	if (res < 0) {
		return res;
	}

	f = file_open("/dev/console", O_W);
	if (!f) {
		return -1;
	}
	p->fdtable[STDOUT] = f;

	f = file_open("/dev/console", O_R);
	if (!f) {
		return -1;
	}
	p->fdtable[STDIN] = f;

	return 0;
}

int process_init(void)
{
	int res;

	_init_proc_table();
	scheduler_init();
	res = _init_idle();

	return res;
}

void process_idle(void)
{
	for (;;) {
		hlt();
	}
}
