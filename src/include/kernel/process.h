/**
 * @file
 * @brief Process control block and process table handling.
 */

#pragma once
#include <clist.h>
#include "trap.h"
#include "waitqueue.h"
#include "vpages.h"
#include <stdbool.h>

#define MAXNAME 16 ///< Size of the process name buffer, in bytes.
#define KSTACK_SIZE 512 ///< Size of a process stack, in 64 bits words.

/** @brief Lifecycle states of a process. */
typedef enum {
	NEW = 0,
	RUNNING,
	READY,
	SLEEPING,
	BLOCKED,
	TERMINATED,
	ZOMBIE
} state;

typedef struct ctx {
	uint64_t ra; ///> Return adress pointer.
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
	tframe_t *tframe_pa; //< The User trap frame used to save the user context.
	void *ustack_pa; ///< Process ustack physical page.

	clist_node_t proc_node; ///< Process table node.
	clist_node_t ready_node; ///< Scheduler ready queue node.
	clist_node_t
		wait_node; ///< Wait queue node, used to put the process in a zombie, IO or Mutex wait state.
	clist_node_t sleep_node; ///< Sleeping queue node.
	uint64_t wake_up_time; ///< Wake up date, in secondes since boot.

	process_t *parent; ///< Parent process, NULL if orphan.
	wait_queue_t child_wq; ///< Queue blocked on while waiting for a child.
	wait_queue_t zombies; ///< Terminated children waiting to be reaped.
	int8_t exit_code; ///< Code to identify what made our process exit.

	priority priority; ///< Scheduling priority class.
} process_t;

/**
 * @brief Make a process the active one, demoting the previous one to READY.
 *
 * @param next Process to make active.
 */
void process_switch_active(process_t *next);

/**
 * @brief Get the clist holding every living process.
 *
 * @return Head sentinel of the process table clist.
 */
const clist_node_t *process_table_clist();

/**
 * @brief Get the running process.
 *
 * @return Pointer to the active process.
 */
process_t *process_active();

/**
 * @brief Boolean helper to determine if a priority is higher than another.
 *
 * We consider that priorities are sorted decremental.
 *
 * @param prior Priority to test.
 * @param other Priority to compare against.
 * @return 1 if @p prior is higher than @p other, 0 otherwise.
 */
uint8_t priority_higher(priority prior, priority other);

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
void process_idle();

/** @brief Init the process table, the scheduler queues and the idle process. */
void process_init();

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
process_t *process_spawn(void code(), const char *name, priority prior,
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
int8_t process_spawn_foreground(void code(), const char *name, priority prior,
			bool user);

/**
 * @brief Duplicate a process state making a copy of it execution state.
 *
 * @return The child process or NULL if failed.
 */
process_t *process_spawn_child(process_t *parent);
