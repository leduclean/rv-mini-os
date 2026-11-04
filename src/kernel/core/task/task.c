#include <stddef.h>
#include <stdint.h>

#include <lib/clist.h>
#include <lib/err.h>
#include <lib/string.h>
#include <lib/tinyalloc.h>

#include <arch/asm_defs.h>
#include <arch/cpu.h>
#include <arch/trap.h>
#include <kernel/elf.h>
#include <kernel/file.h>
#include <kernel/mmap.h>
#include <kernel/pages.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>
#include <kernel/task.h>
#include <kernel/time.h>
#include <kernel/vpages.h>
#include <kernel/waitqueue.h>
#include <user/kinterface.h>

#include "arch/abi.h"
#include "lib/bit.h"

#define MAX_TASK 32

/** @brief Task table. */
typedef struct {
	uint8_t next_pid; ///< Pid given to the next spawned task.
	uint8_t active_task; ///< Number of living tasks, not TERMINATED.
	clist_node_t head; ///< Head sentinel of the living tasks clist.
} ptable_t;

/** @brief The one task table. */
static ptable_t task_table;

/** @brief Currently running task. */
static task_t *active = NULL;

/** @brief Init the task table. */
static void init_task_table(void)
{
	clist_init_node(&task_table.head);
	task_table.next_pid = 0;
	task_table.active_task = 0;
}

/**
 * @brief Remove a task from all the blocking queues it could wait on.
 *
 * @note This function is **not atomic**. The caller MUST ensure atomicity.
 *
 * @param task Task to clear.
 */
static void remove_from_blocking_queues(task_t *task)
{
	if (clist_is_in_list(&task->sleep_node))
		clist_remove(&task->sleep_node);
	if (clist_is_in_list(&task->wait_node)) {
		clist_remove(&task->wait_node);
	}
}

/**
 * @brief Remove a task from all the scheduling queues it could be in.
 *
 * @note This function is **not atomic**. The caller MUST ensure atomicity.
 *
 * @param task Task to remove from the queues.
 */
static inline void remove_from_sched_queues(task_t *task)
{
	if (clist_is_in_list(&task->ready_node))
		clist_remove(&task->ready_node);
	remove_from_blocking_queues(task);
}

/**
 * @brief Remove a task from all the queues it could be in.
 *
 * @note This function is **not atomic**. The caller MUST ensure atomicity.
 *
 * @param task Task to remove from the queues.
 */
static inline void remove_from_all_queues(task_t *task)
{
	if (clist_is_in_list(&task->task_node))
		clist_remove(&task->task_node);
	remove_from_sched_queues(task);
}

/**
 * @brief Reset all the nodes and the wait queues of a task.
 *
 * @param task Task to reset.
 */
static inline void init_task_queues(task_t *task)
{
	clist_init_node(&task->task_node);
	clist_init_node(&task->ready_node);
	clist_init_node(&task->wait_node);
	clist_init_node(&task->sleep_node);
	clist_init_node(&task->zombies);
	wq_init(&task->zombie_wq);
}

/**
 * @brief Alloc a task squeleton ie without activating it or setting context.
 * 
 * @param name Name of the task. 
 * @param prior Priority of the task.
 * @param user User task flag.
 * @param parent Parent of the task.
 * @return The new task.
 * @retval ERR_PTR(-EAGAIN) Task table full.
 * @retval ERR_PTR(-ENOMEM) Out of memory.
 */
static inline task_t *alloc_squeletton(const char *name, priority prior,
				       bool user, task_t *parent)
{
	if (task_table.active_task >= MAX_TASK) {
		return ERR_PTR(-EAGAIN);
	}

	task_t *task = calloc(1, sizeof(task_t));
	if (!task) {
		return ERR_PTR(-ENOMEM);
	}

	init_task_queues(task);

	strncpy(task->name, name, sizeof(task->name) - 1);
	task->user = user;
	task->parent = parent;
	task->priority = prior;
	task->pid = task_table.next_pid++;
	task->tgid = task->pid;
	task->state = NEW;

	return task;
}

/**
 * @brief Activate with the scheduler a new task.
 *
 * @param p A pointer to the task.
 */
static void activate_task(task_t *p)
{
	p->state = READY;
	clist_push_back(&task_table.head, &p->task_node);
	task_table.active_task++;
	scheduler_admit(p);
	return;
}

/**
 * @brief Kernel privilege proessus entry point.
 */
static void ktask_launcher(void)
{
	//NOTE: We force this because there is no guarantee,
	// after a ktask spawn to have irq enable.
	// (ctx switch does not preserve the sstatus)
	irq_enable_s();
	task_t *p = task_active();
	p->code();
	scheduler_exit_group(0);
}

/** @brief Create the idle task and make it active. */
static int init_idle(void)
{
	task_t *p = task_kthread(task_idle, "idle", IDLE);
	if (IS_ERR(p)) {
		return PTR_ERR(p);
	}
	active = p;
	active->state = RUNNING;
	return 0;
}

task_t *task_active(void)
{
	return active;
}

void task_switch_active(task_t *next)
{
	if (active->state == RUNNING)
		active->state = READY;

	next->state = RUNNING;
	active = next;
}

const clist_node_t *task_table_clist(void)
{
	return &task_table.head;
}

uint8_t priority_higher(priority prior, priority other)
{
	return prior < other;
}

void task_sleep(task_t *task, uint32_t delay)
{
	task->state = SLEEPING;
	task->wake_up_time = delay + time_seconds();
}

void task_block(task_t *task)
{
	task->state = BLOCKED;
}

void task_wake(task_t *task)
{
	irq_flags_t state = irq_save();
	if ((task->state == SLEEPING) || (task->state == BLOCKED)) {
		// Remove it from sleeping and blocked queue if remaining
		remove_from_blocking_queues(task);
		task->state = RUNNING;
	}
	irq_restore(state);
}

static void release_fds(task_t *p)
{
	for (int i = 0; i < NOFILE; i++) {
		struct file *f = p->fdtable[i];
		if (f) {
			file_put(f);
		}
	}
}

void task_zombify(task_t *task, int exit_code)
{
	irq_flags_t state = irq_save();

	task_t *parent = task->parent;
	if (parent == NULL) {
		panic("Zombify an orphan task is not permitted");
	}

	task->state = ZOMBIE;
	task->exit_code = exit_code;

	remove_from_sched_queues(task);
	release_fds(task);
	clist_push_back(&parent->zombies, &task->wait_node);

	if (parent->state == BLOCKED) {
		// Parent is waiting so we wake him up
		// to check if he can stop wait.
		scheduler_wake_waiting_queue(&parent->zombie_wq);
	}

	irq_restore(state);
}

void task_destroy(task_t *task)
{
	irq_flags_t state = irq_save();
	if (task_active() == task) {
		panic("Trying to destroy the current task led to segfault. Use zombify instead");
	}

	task->state = TERMINATED;
	remove_from_all_queues(task);

	pte_t *root = task->root_ptable;
	if (task->user && root) {
		vpage_unmmap(task->root_ptable, (void *)task->tframe_va);
		if (page_get_ref_count(root) == 1) {
			vpage_tree_free(task->root_ptable);
		} else {
			page_put(root);
		}
	}

	free(task);
	task_table.active_task--;

	irq_restore(state);
}

static void copy_fds(task_t *child, const task_t *parent, bool clone)
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

static int load_elf_image(task_t *p, struct file *elf, uintptr_t *entry)
{
	int res;
	struct elf_hdr hdr;
	res = elf_load_hdr(elf, &hdr);
	if (res < 0) {
		return res;
	}

	res = elf_verify_file_format(&hdr);
	if (res < 0) {
		return res;
	}

	res = mmap_uimage(p);
	if (res < 0) {
		return res;
	}

	res = elf_map(p->root_ptable, elf, &hdr);
	if (res < 0) {
		vpage_tree_free(p->root_ptable);
		return res;
	}

	*entry = hdr.entry;

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

	return -E2BIG;
}

static int transfer_argv(pte_t *old_ptable, pte_t *new_ptable,
			 char *const argv[], uintptr_t *out_sp)
{
	ssize_t len;
	uintptr_t sp = USTACK;
	char *argv_k[EXEC_MAX_ARGS];
	int argc = get_argc(old_ptable, argv, argv_k);
	if (argc < 0) {
		return argc;
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

task_t *task_start_userland(struct file *user_elf, const char *name,
			    priority prior)
{
	irq_flags_t state = irq_save();
	task_t *p = alloc_squeletton(name, prior, true, task_active());
	if (IS_ERR(p)) {
		goto err_restore_irq;
	}

	p->ctx.sp = (uint64_t)&p->kstack[KSTACK_SIZE];
	p->ctx.ra = (uintptr_t)trap_return_to_user;

	uintptr_t entry;
	int res = load_elf_image(p, user_elf, &entry);
	if (res < 0) {
		goto err_free_task;
	}

	uintptr_t sp;
	res = transfer_argv(NULL, p->root_ptable, NULL, &sp);
	if (res < 0) {
		goto err_free_task;
	}

	tframe_init(p->tframe_pa, p->root_ptable,
		    (unsigned long)&p->kstack[KSTACK_SIZE]);
	tframe_start(p->tframe_pa, (void *)entry, (void *)sp);

	copy_fds(p, task_active(), false);
	activate_task(p);
	irq_restore(state);
	return p;

err_free_task:
	free(p);
	p = ERR_PTR(res);
err_restore_irq:
	irq_restore(state);
	return p;
}

task_t *task_kthread(void code(void), const char *name, priority prior)
{
	irq_flags_t state = irq_save();
	task_t *p = alloc_squeletton(name, prior, false, task_active());
	if (IS_ERR(p)) {
		goto err_restore_irq;
	}

	p->code = code;
	if (p->pid == 0) {
		p->ctx.ra = (uintptr_t)code;
	} else {
		p->ctx.sp = (uint64_t)&p->kstack[KSTACK_SIZE];
		p->ctx.ra = (uintptr_t)ktask_launcher;
	}

	copy_fds(p, task_active(), false);
	activate_task(p);
	irq_restore(state);
	return p;

err_restore_irq:
	irq_restore(state);
	return p;
}

task_t *task_clone(task_t *parent, void *entry, void *args, void *stack,
		   unsigned long flags)
{
	if (!parent->user) {
		panic("Only fork for user task is supported ");
	}

	int res = -ENOMEM;
	irq_flags_t state = irq_save();
	task_t *child = alloc_squeletton(parent->name, parent->priority,
					 parent->user, parent);
	if (IS_ERR(child)) {
		goto err_restore_irq;
	}

	tframe_t *t = page_alloc();
	if (!t) {
		goto err_free_task;
	}
	child->tframe_pa = t;

	if (flags & CLONE_VM) {
		// Add a reference to this page.
		child->root_ptable = page_get((void *)parent->root_ptable);
		child->tgid = parent->tgid;

		if (!stack || !entry) {
			res = -EINVAL;
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
		res = vpage_tree_copy(root, parent->root_ptable);
		if (res < 0) {
			goto err_free_map;
		}

		// Copy the trap frame of the parent to continue at the same state.
		memcpy(t, parent->tframe_pa, sizeof(*child->tframe_pa));
		// Child returns 0 from the original state. (ie fork() return 0 in the child)
		tframe_set_return_val(t, 0);
		child->tframe_va = TRAPFRAME;
	}

	// Map the trap frame into the table
	res = vpage_map(child->root_ptable, (void *)child->tframe_va, t,
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

	activate_task(child);
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
err_free_task:
	free(child);
	child = ERR_PTR(res);
err_restore_irq:
	irq_restore(state);
	return child;
}

task_t *task_fork(task_t *parent)
{
	return task_clone(parent, NULL, NULL, NULL, 0);
}

int task_exec(task_t *p, struct file *elf, char *const argv[])
{
	int res;

	if (!p->user) {
		panic("Task exec on non user is not supported");
	}

	// Save the state
	irq_flags_t state = irq_save();
	pte_t *old_ptable = p->root_ptable;
	tframe_t *old_tframe = p->tframe_pa;

	uintptr_t entry;
	res = load_elf_image(p, elf, &entry);
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
	tframe_start(p->tframe_pa, (void *)entry, (void *)sp);

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

int task_give_fd(task_t *p, struct file *f)
{
	for (int fd = 0; fd < NOFILE; fd++) {
		if (p->fdtable[fd] == NULL) {
			p->fdtable[fd] = f;
			return fd;
		}
	}

	return -EMFILE;
}

struct file *task_get_fd(const task_t *p, int fd)
{
	if (fd >= NOFILE || fd < 0) {
		return NULL;
	}
	return p->fdtable[fd];
}

int task_release_fd(task_t *p, int fd)
{
	struct file *f = p->fdtable[fd];
	if (!f) {
		return -EBADF;
	}

	p->fdtable[fd] = NULL;
	file_put(f);

	return 0;
}

int task_map_console(task_t *p)
{
	struct file *f;
	int res = fs_mknod("/dev/console", FILE_DEV_CONSOLE);
	if (res < 0) {
		return res;
	}

	f = file_open("/dev/console", O_W);
	if (IS_ERR(f)) {
		return PTR_ERR(f);
	}
	p->fdtable[STDOUT] = f;

	f = file_open("/dev/console", O_R);
	if (IS_ERR(f)) {
		return PTR_ERR(f);
	}

	p->fdtable[STDIN] = f;

	return 0;
}

int task_init(void)
{
	int res;

	init_task_table();
	scheduler_init();
	res = init_idle();

	return res;
}

void task_idle(void)
{
	for (;;) {
		hlt();
	}
}
