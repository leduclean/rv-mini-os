/**
 * @file
 * @brief Process control block and process table handling.
 */

#pragma once
#include "clist.h"
#include "tinyalloc.h"
#include "trap.h"
#include "waitqueue.h"
#include "vpages.h"

#define MAXNAME 16 ///< Size of the process name buffer, in bytes.
#define KSTACK_SIZE 512 ///< Size of a process stack, in 64 bits words.

/** @brief Lifecycle states of a process. */
typedef enum {
	FREE = 0,
	RUNNING,
	READY,
	SLEEPING,
	BLOCKED,
	TERMINATED,
	ZOMBIE
} state;

typedef struct ctx {
	uint64_t ra; ///> Return adress pointer pointing to proc_launcher()
	uint64_t sp; ///> Stack pointer of the process.
	uint64_t s[12]; ///> Callee saved registry.
} ctx_t;

/* ctxt.S hardcodes these offsets, keep both in sync. */
_Static_assert(__builtin_offsetof(ctx_t, s) == 2 * 8, "ctxt.S offsets stale");

/** @brief Scheduling priorities, sorted decremental (HIGH is the highest). */
typedef enum { HIGH = 0, NORMAL, LOW, IDLE, PRIORITY_COUNT } priority;

/** @brief Process control block. */
typedef struct process {
	uint8_t pid; ///< Process identifier.
	void (*code)(); ///< Process code to launch.
	char name[MAXNAME]; ///< Process name, null terminated.
	state state; ///< Current lifecycle state.
	ctx_t ctx; ///< Registers saved on a context switch.
	uint64_t kstack[KSTACK_SIZE]; ///< Process stack.
	pte_t *root_ptable; ///< Process Root Page table.
	bool user; ///< User mode Process flag (fixed at creation).
	tframe_t *tframe; //< The User trap frame used to save the user context.

	clist_node_t proc_node; ///< Process table node.

	clist_node_t ready_node; ///< Scheduler ready queue node.

	/**
   * @brief Wait queue node, used to put the process in a zombie, IO or
   * mutex wait.
   */
	clist_node_t wait_node;
	wait_queue_t *current_wq; ///< Wait queue blocked on, NULL if not blocked.

	clist_node_t sleep_node; ///< Sleeping queue node.
	uint64_t wake_up_time; ///< Wake up date, in secondes since boot.

	process_t *parent; ///< Parent process, NULL if orphan.
	wait_queue_t child_wq; ///< Queue blocked on while waiting for a child.
	wait_queue_t zombies; ///< Terminated children waiting to be reaped.
	int exit_code; ///< Code to identify what made our process exit.

	priority priority; ///< Scheduling priority class.
} process_t;

/**
 * @brief Get the saved register context of a process.
 *
 * @param proc Process to read.
 * @return Pointer to its saved context.
 */
ctx_t *get_ctx(process_t *proc);

/**
 * @brief Get the wake up time of a process.
 *
 * @param proc Process to read.
 * @return Wake up time, in secondes since boot.
 */
uint32_t get_wake_up(const process_t *proc);

/**
 * @brief Get the priority of a process.
 *
 * @param proc Process to read.
 * @return Priority class of @p proc.
 */
priority get_priority(const process_t *proc);

/**
 * @brief Get the name of a process.
 *
 * @param proc Process to read.
 * @return Pointer to the name of @p proc.
 */
const char *get_name(const process_t *proc);

/**
 * @brief Get the pid of a process.
 *
 * @param proc Process to read.
 * @return Pid of @p proc.
 */
uint8_t get_pid(const process_t *proc);

/**
 * @brief Get the ready queue node of a process.
 *
 * @param proc Process to read.
 * @return Pointer to its ready queue node.
 */
clist_node_t *get_ready_node(process_t *proc);

/**
 * @brief Get the wait queue node of a process.
 *
 * @param proc Process to read.
 * @return Pointer to its wait queue node.
 */
clist_node_t *get_wait_node(process_t *proc);

/**
 * @brief Get the sleeping queue node of a process.
 *
 * @param proc Process to read.
 * @return Pointer to its sleeping queue node.
 */
clist_node_t *get_sleep_node(process_t *proc);

/**
 * @brief Get the queue holding the zombie children of a process.
 *
 * @param proc Parent process.
 * @return Pointer to its zombies queue.
 */
wait_queue_t *get_zombies(process_t *proc);

/**
 * @brief Get the queue a process blocks on while waiting for a child.
 *
 * @param proc Parent process.
 * @return Pointer to its child wait queue.
 */
wait_queue_t *get_wait_child_queue(process_t *proc);

/**
 * @brief Set the state of a process.
 *
 * @param proc Process to update.
 * @param state New state.
 */
void set_state(process_t *proc, state state);

/**
 * @brief Make a process the active one, demoting the previous one to READY.
 *
 * @param next Process to make active.
 */
void switch_active(process_t *next);

/**
 * @brief Get the clist holding every living process.
 *
 * @return Head sentinel of the process table clist.
 */
const clist_node_t *get_proc_table_clist();

/**
 * @brief Get the running process.
 *
 * @return Pointer to the active process.
 */
process_t *get_active();

/**
 * @brief Get the pid of the running process.
 *
 * @return Pid of the active process.
 */
uint8_t get_active_pid();

/**
 * @brief Get the name of the running process.
 *
 * @return Pointer to the name of the active process.
 */
char *get_active_name();

/**
 * @brief Boolean helper to determine if a priority is higher than another.
 *
 * We consider that priorities are sorted decremental.
 *
 * @param prior Priority to test.
 * @param other Priority to compare against.
 * @return 1 if @p prior is higher than @p other, 0 otherwise.
 */
uint8_t higher_priority(priority prior, priority other);

/**
 * @brief Set a process in sleeping state with a timer.
 *
 * @param proc Process to put to sleep.
 * @param delay Sleeping duration, in secondes.
 */
void process_sleep(process_t *proc, uint32_t delay);

/**
 * @brief Set a process to blocked state, waiting for an IO irq.
 *
 * @param proc Process to block.
 */
void process_block(process_t *proc);

/**
 * @brief Switch a sleeping or blocked process back to running.
 *
 * @note It is also removed from every blocking queue it was in.
 *
 * @param proc Process to wake up.
 */
void process_wake(process_t *proc);

/**
 * @brief Terminate a process.
 *
 * @note It is zombified if it has a parent, cleaned up otherwise.
 *
 * @param proc Process to terminate.
 * @param exit_code 0 if the process finished normally else exit code.
 */
void process_terminate(process_t *proc, int exit_code);

/**
 * @brief Reap a zombie process, called by its parent.
 *
 * @param proc Zombie process to reap.
 */
void process_reap(process_t *proc);

/** @brief Idle process, runs whenever no other process is ready. */
void idle();

/** @brief Init the process table, the scheduler queues and the idle process. */
void init_proc();

/**
 * @brief Spawn a process and admit it in the scheduler.
 *
 * @param code Entry point of the process.
 * @param name Name of the process, truncated to MAXNAME - 1 chars.
 * @param prior Priority of the process.
 * @param user User mode process flag.
 * @return Pointer to the spawned process, NULL if the table is full or the
 * allocation failed.
 */
process_t *spawn_process(void code(), const char *name, priority prior,
			 bool user);

/**
 * @brief Spawn a child process and block until it terminates.
 *
 * @param code Entry point of the child.
 * @param name Name of the child, truncated to MAXNAME - 1 chars.
 * @param prior Priority of the child.
 * @param user User mode process flag.
 * @return Pid of the child, -1 if the spawn failed.
 */
int8_t spawn_foreground(void code(), const char *name, priority prior,
			bool user);
