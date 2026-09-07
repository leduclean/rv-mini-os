#include "process.h"
#include "clist.h"
#include "kernel_config.h"
#include "mmap.h"
#include "pages.h"
#include "scheduler.h"
#include "sync.h"
#include "time.h"
#include "waitqueue.h"
#include "vpages.h"

#if TEST_CONFIG
#include "mocks/kernel_mocks.h"
#include <stddef.h>
#include <stdlib.h>
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

// Getters and setter on fields
ctx_t *get_ctx(process_t *proc)
{
	return &proc->ctx;
}

uint8_t get_pid(const process_t *proc)
{
	return proc->pid;
};
const char *get_name(const process_t *proc)
{
	return proc->name;
}
uint32_t get_wake_up(const process_t *proc)
{
	return proc->wake_up_time;
}
priority get_priority(const process_t *proc)
{
	return proc->priority;
}
wait_queue_t *get_wait_child_queue(process_t *proc)
{
	return &proc->child_wq;
};
wait_queue_t *get_zombies(process_t *proc)
{
	return &proc->zombies;
};
void set_state(process_t *proc, state state)
{
	proc->state = state;
}

// Node getters
clist_node_t *get_ready_node(process_t *proc)
{
	return &proc->ready_node;
}
clist_node_t *get_wait_node(process_t *proc)
{
	return &proc->wait_node;
}
clist_node_t *get_sleep_node(process_t *proc)
{
	return &proc->sleep_node;
}
wait_queue_t *get_current_wq(process_t *proc)
{
	return proc->current_wq;
}

/** @brief Currently running process. */
static process_t *active = NULL;

// Active getters
process_t *get_active()
{
	return active;
}
uint8_t get_active_pid()
{
	return active->pid;
}
char *get_active_name()
{
	return active->name;
}

void switch_active(process_t *next)
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

	page_free(proc->root_ptable);
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
static inline void _process_zombify(process_t *proc)
{
	irq_flags_t state = irq_save();

	process_t *parent = proc->parent;
	if (!parent)
		return;

	proc->state = ZOMBIE;

	wq_enqueue(&parent->zombies, &proc->wait_node);
	if (parent->state == BLOCKED) {
		// Parent is waiting so we wake him up
		// to check if he can stop wait.
		scheduler_wake_waiting_queue(&parent->child_wq);
	}

	irq_restore(state);
}

void process_terminate(process_t *proc)
{
	process_t *parent = proc->parent;
	if (parent) {
		_process_zombify(proc);
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

uint8_t higher_priority(priority prior, priority other)
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

static pte_t *_allocate_root_ptable(process_t *proc)
{
	pte_t *root = page_alloc();
	proc->root_ptable = root;
	return root;
}

/**
 * @brief Config a process in its slot of the process table.
 *
 * @param code Entry point of the process.
 * @param nom Name of the process, truncated to MAXNAME - 1 chars.
 * @param prior Priority of the process.
 * @param proc Process slot to configure.
 */
static void _config_process(void code(), const char *name, priority prior,
			    process_t *proc)
{
	_init_process_queues(proc);
	proc->pid = proc_table.next_pid;
	strncpy(proc->name, name, MAXNAME - 1);
	proc->priority = prior;
	proc->state = READY;
	proc->parent = NULL;

	if (proc->pid == 0) {
		proc->ctx.ra = (uintptr_t)idle;
	} else {
		proc->ctx.sp = (uint64_t)&proc->kstack[KSTACK_SIZE];
		proc->ctx.ra = (uintptr_t)proc_launcher;
		proc->code = code;
	}
}

/**
 * @brief Add a process to the running queue.
 *
 * @param slot Process to admit.
 * @return Pointer to the admitted process.
 */
static process_t *_add_process_to_scheduler(process_t *slot)
{
	proc_table.active_process++;
	proc_table.next_pid++;
	scheduler_admit(slot);
	return slot;
}

process_t *spawn_process(void code(), const char *nom, priority prior,
			 bool user)
{
	irq_flags_t state = irq_save();

	if (proc_table.active_process >= MAX_PROC) {
		return NULL; // Error already max processus launched
	}

	process_t *proc = calloc(1, sizeof(process_t));
	if (!proc) {
		return NULL;
	}

	pte_t *ptable = _allocate_root_ptable(proc);
	if (!ptable) {
		goto err_free_proc;
	}

	_config_process(code, nom, prior, proc);

	proc->user = user;
	if (user) {
		if (map_uprocess(proc) != 0) {
			goto err_free_proc;
		}
	}

	clist_push_back(&proc_table.head, &proc->proc_node);
	process_t *spawned = _add_process_to_scheduler(proc);

	irq_restore(state);
	return spawned;

err_free_proc:
	free(proc);
	return NULL;
}

int8_t spawn_foreground(void code(), const char *name, priority prior,
			bool user)
{
	process_t *parent = get_active();
	process_t *child = spawn_process(code, name, prior, user);
	if (!child) {
		return -1;
	}
	child->parent = parent;
	uint8_t pid = child->pid;
	wait_pid(child->pid);
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
