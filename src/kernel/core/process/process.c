#include "process.h"
#include "clist.h"
#include "kernel_config.h"
#include "mmap.h"
#include "syscall.h"
#include "pages.h"
#include "scheduler.h"
#include "time.h"
#include "trap.h"
#include "waitqueue.h"
#include "vpages.h"

#if TEST_CONFIG
#include "mocks/kernel_mocks.h"
#include "minilib/stddef.h"
#include <string.h>
#else
#include "cpu.h"
#include "minilib/stddef.h"
#include "minilib/string.h"
#include "tinyalloc.h"
#endif

#define MAX_PROC 32

/** @brief Process table. */
typedef struct {
	uint8_t next_pid; ///< Pid given to the next spawned process.
	uint8_t active_process; ///< Number of living processes, not TERMINATED.
	clist_node_t head; ///< Head sentinel of the living processes clist.
} ptable_t;

static ptable_t proc_table;
const clist_node_t *get_proc_table_clist()
{
	return &proc_table.head;
}

/** @brief Init the process table. */
static void _init_proc_table()
{
	clist_init_node(&proc_table.head);
	proc_table.next_pid = 0;
	proc_table.active_process = 0;
}

/** @brief Currently running process. */
static process_t *active = NULL;

process_t *process_active()
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

/**
 * @brief Remove a process from all the blocking queues it could wait on.
 *
 * @note This function is **not atomic**. The caller MUST ensure atomicity.
 *
 * @param proc Process to clear.
 */
static void _clear_from_blocking_queues(process_t *proc)
{
	irq_flags_t state = irq_save();
	// Remove it if it from sleeping queue (timeout).
	if (is_in_sleeping_queue(proc)) {
		remove_from_sleeping(proc);
	};

	if (clist_is_in_list(&proc->wait_node)) {
		clist_remove(&proc->wait_node);
	}
	irq_restore(state);
}

// State handling
void process_sleep(process_t *proc, uint32_t delay)
{
	proc->state = SLEEPING;
	proc->wake_up_time = delay + seconds();
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
		_clear_from_blocking_queues(proc);
		proc->state = RUNNING;
	}
	irq_restore(state);
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
	if (clist_is_in_list(&proc->ready_node))
		clist_remove(&proc->ready_node);
	if (clist_is_in_list(&proc->sleep_node))
		clist_remove(&proc->sleep_node);
	if (clist_is_in_list(&proc->wait_node))
		clist_remove(&proc->wait_node);
}
/**
 * @brief Clean a process up, removing it from all queues and from memory.
 *
 * @param proc Process to clean up.
 */
static inline void _process_clean_up(process_t *proc)
{
	irq_flags_t state = irq_save();

	proc->state = TERMINATED;
	_remove_from_all_queues(proc);

	if (proc->user && proc->root_ptable) {
		tree_free(proc->root_ptable);
	}

	free(proc);
	proc_table.active_process--;

	irq_restore(state);
}

/**
 * @brief Zombify a process, putting it in the zombies queue of its parent.
 *
 * @note The parent is woken up if it was waiting for a child.
 *
 * @param proc Process to zombify.
 */
static inline void _process_zombify(process_t *proc, int exit_code)
{
	irq_flags_t state = irq_save();

	// ponytail: caller (process_terminate) already proved parent != NULL
	process_t *parent = proc->parent;

	proc->state = ZOMBIE;
	proc->exit_code = exit_code;

	wq_enqueue(&parent->zombies, &proc->wait_node);
	if (parent->state == BLOCKED) {
		// Parent is waiting so we wake him up
		// to check if he can stop wait.
		scheduler_wake_waiting_queue(&parent->child_wq);
	}

	irq_restore(state);
}

void process_terminate(process_t *proc, int exit_code)
{
	process_t *parent = proc->parent;
	if (parent) {
		_process_zombify(proc, exit_code);
	} else {
		_process_clean_up(proc);
	}
}

void process_reap(process_t *proc)
{
	if (proc->state != ZOMBIE) {
		return;
	}
	_process_clean_up(proc);
}

uint8_t priority_higher(priority prior, priority other)
{
	return prior < other;
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
 * @brief Alloc and set the page of the root page table of a processsus.
 *
 * @param proc A pointer to the processus.
 * @return The allocated page on success, NULL on failure.
 */
static pte_t *_alloc_root_ptable(process_t *proc)
{
	pte_t *root = page_alloc();
	proc->root_ptable = root;
	return root;
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

	pte_t *ptable = _alloc_root_ptable(proc);
	if (!ptable) {
		goto err_free_proc;
	}

	strncpy(proc->name, name, sizeof(proc->name) - 1);
	proc->user = user;
	proc->priority = prior;
	proc->parent = parent;
	proc->pid = proc_table.next_pid++;
	proc->state = NEW;
	return proc;

err_free_proc:
	free(proc);
	return NULL;
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
static void _kproc_launcher()
{
	//NOTE: We force this because there is no guarantee,
	// after a kprocess spawn to have irq enable.
	// (ctx switch does not preserve the sstatus)
	enable_s_irq();
	process_t *p = process_active();
	p->code();
	scheduler_terminate(0);
}

process_t *spawn_process(void code(), const char *name, priority prior,
			 bool user)
{
	irq_flags_t state = irq_save();
	process_t *p = _alloc_squeletton(name, prior, user, process_active());
	if (!p) {
		goto err_restore_irq;
	}

	if (p->pid == 0) {
		p->ctx.ra = (uintptr_t)code;
	} else {
		p->ctx.sp = (uint64_t)&p->kstack[KSTACK_SIZE];
		p->ctx.ra = user ? (uintptr_t)enter_user_mode :
				   (uintptr_t)_kproc_launcher;
		p->code = code;
	}

	if (user && map_uprocess(p) != 0) {
		goto err_free_ptable;
	}

	_activate_process(p);
	irq_restore(state);
	return p;

err_free_ptable:
	tree_free(p->root_ptable);
	free(p);
	p = NULL;

err_restore_irq:
	irq_restore(state);
	return p;
}

static inline void _fork_return()
{
	process_t *p = process_active();
	get_trap_return_va()(p->tframe_pa->saved_regs.satp);
}

process_t *spawn_child(process_t *parent)
{
	irq_flags_t state = irq_save();

	process_t *child =
		_alloc_squeletton(parent->name, parent->priority, true, parent);
	if (!child || !parent->user) {
		goto err_restore_irq;
	}

	// Copy the tree
	if (tree_copy(child->root_ptable, parent->root_ptable) < 0) {
		goto err_free_tree;
	}

	// Allocate a page for the trap frame
	void *tframe = page_alloc();
	if (!tframe) {
		goto err_free_tree;
	}
	if (map_page(child->root_ptable, (void *)TRAPFRAME, tframe,
		     PTE_R | PTE_W) < 0) {
		goto err_free_tframe;
	}
	child->tframe_pa = tframe;

	// Copy the trap frame of the parent
	memcpy(child->tframe_pa, parent->tframe_pa, sizeof(*child->tframe_pa));

	// Child returns 0 from fork()
	// This is going to be restored from trap_return
	child->tframe_pa->saved_regs.a[0] = 0;
	child->tframe_pa->saved_regs.satp = get_satp(child->root_ptable);
	child->tframe_pa->kstack = (unsigned long)&child->kstack[KSTACK_SIZE];

	// On ctx switch on fork, we want this state
	child->ctx.ra = (unsigned long)_fork_return;
	child->ctx.sp = (unsigned long)&child->kstack[KSTACK_SIZE];

	_activate_process(child);
	irq_restore(state);
	return child;

err_free_tframe:
	page_put(tframe);
err_free_tree:
	tree_free(child->root_ptable);
	free(child);
	child = NULL;
err_restore_irq:
	irq_restore(state);
	return child;
}

int8_t spawn_foreground(void code(), const char *name, priority prior,
			bool user)
{
	process_t *child = spawn_process(code, name, prior, user);
	if (!child) {
		return -1;
	}
	uint8_t pid = child->pid;
	// Wait for the child to terminate.
	// FIX: It's weird that there is some
	// syscall here.
	sys_wait_pid(child->pid);
	return pid;
};

/** @brief Create the idle process and make it active. */
static void _init_idle()
{
	process_t *init_proc = spawn_process(idle, "idle", IDLE, false);
	if (!init_proc) {
		//TODO: handle error
		return;
	}
	active = init_proc;
	active->state = RUNNING;
}

void init_proc()
{
	_init_proc_table();
	init_scheduler_queues();
	_init_idle();
}

void idle()
{
	for (;;) {
		hlt();
	}
}
