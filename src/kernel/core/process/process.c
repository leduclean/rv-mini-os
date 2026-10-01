#include <stddef.h>
#include <stdint.h>

#include <lib/clist.h>
#include <lib/string.h>
#include <lib/tinyalloc.h>

#include <asm/asm_defs.h>
#include <asm/cpu.h>
#include <asm/trap.h>

#include <kernel/mmap.h>
#include <kernel/pages.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>
#include <kernel/time.h>
#include <kernel/vpages.h>
#include <kernel/waitqueue.h>
#include <user/user_entry.h>

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
	wq_init(&proc->zombies);
	wq_init(&proc->child_wq);
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
	proc->priority = prior;
	proc->parent = parent;
	proc->pid = proc_table.next_pid++;
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
static void _init_idle(void)
{
	process_t *p = process_spawn(process_idle, "idle", IDLE, false);
	if (!p) {
		//TODO: handle error
		return;
	}
	active = p;
	active->state = RUNNING;
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
	wq_enqueue(&parent->zombies, &proc->wait_node);

	if (parent->state == BLOCKED) {
		// Parent is waiting so we wake him up
		// to check if he can stop wait.
		scheduler_wake_waiting_queue(&parent->child_wq);
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

process_t *process_spawn(void code(void), const char *name, priority prior,
			 bool user)
{
	irq_flags_t state = irq_save();
	process_t *p = _alloc_squeletton(name, prior, user, process_active());
	if (!p) {
		goto err_restore_irq;
	}

	p->code = code;
	p->tgid = p->pid;
	if (p->pid == 0) {
		p->ctx.ra = (uintptr_t)code;
	} else {
		p->ctx.sp = (uint64_t)&p->kstack[KSTACK_SIZE];
		p->ctx.ra = user ? (uintptr_t)trap_return_to_user
				 : (uintptr_t)_kproc_launcher;
	}

	if (user) {
		if (mmap_uimage(p) != 0) {
			goto err_free_proc;
		}
		tframe_init(p->tframe_pa, p->root_ptable,
			    (unsigned long)&p->kstack[KSTACK_SIZE]);
		tframe_start(p->tframe_pa, user_entry_point, (void *)USTACK);
		tframe_set_arg(p->tframe_pa, 0, (unsigned long)code);
	}

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

int process_exec(process_t *p, void code(void))
{
	if (!p->user) {
		panic("Process exec on non user is not supported");
	}
	// Save the state
	irq_flags_t state = irq_save();
	pte_t *old_ptable = p->root_ptable;
	tframe_t *old_tframe = p->tframe_pa;

	int res = 0;
	res = mmap_uimage(p);
	if (res < 0) {
		goto err_restore_old_state;
	}

	p->code = code;
	tframe_init(p->tframe_pa, p->root_ptable,
		    (unsigned long)&p->kstack[KSTACK_SIZE]);
	tframe_start(p->tframe_pa, user_entry_point, (void *)USTACK);
	tframe_set_arg(p->tframe_pa, 0, (unsigned long)code);

	// Once all has worked free all the old ressources.
	vpage_tree_free(old_ptable);
	irq_restore(state);

	return res;

err_restore_old_state:
	p->root_ptable = old_ptable;
	p->tframe_pa = old_tframe;
	irq_restore(state);
	return res;
}

int8_t process_spawn_foreground(void code(void), const char *name,
				priority prior, bool user)
{
	process_t *child = process_spawn(code, name, prior, user);
	if (!child) {
		return -1;
	}
	uint8_t pid = child->pid;
	// Wait for the child to terminate.
	// FIX: It's weird that there is some
	// syscall here.
	sys_waitpid(child->pid);
	return pid;
};

void process_init(void)
{
	_init_proc_table();
	scheduler_init();
	_init_idle();
}

void process_idle(void)
{
	for (;;) {
		hlt();
	}
}
