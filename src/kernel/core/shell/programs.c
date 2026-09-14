#include "programs.h"
#include "apps.h"
#include <clist.h>
#include "cmd_registry.h"
#include <container.h>
#include <stddef.h>
#include <stdio.h>
#include <stdbool.h>
#include "mutex.h"
#include "process.h"
#include "scheduler.h"
#include "time.h"

#define MAX_CMDS 64

// /** @brief Shared mutex for the mutex demo. */
// static mutex_t shared_mutex;
// static uint8_t mutex_initialized = 0;
//
// /** @brief Init the shared mutex on the first demo use. */
// static void _ensure_mutex_init()
// {
// 	if (!mutex_initialized) {
// 		mutex_init(&shared_mutex);
// 		mutex_initialized = 1;
// 	}
// }
//
// /* --- Demo processes --- */
//
// static void _proc1()
// {
// 	printf("[proc1] started (pid=%d, priority=HIGH)\n",
// 	       process_active()->pid);
// 	for (int i = 0; i < 3; i++) {
// 		printf("[proc1] running at t=%us — sleeping 5s...\n",
// 		       time_seconds());
// 		scheduler_sleep(5);
// 	}
// 	printf("[proc1] done, exiting\n");
// }
//
// static void _proc3()
// {
// 	printf("[proc3] spawned dynamically by proc2 (pid=%d, priority=LOW)\n",
// 	       process_active()->pid);
// 	for (;;) {
// 		printf("[proc3] alive at t=%us — sleeping 4s\n",
// 		       time_seconds());
// 		scheduler_sleep(4);
// 	}
// }
//
// static void _proc2()
// {
// 	printf("[proc2] started (pid=%d, priority=NORMAL)\n",
// 	       process_active()->pid);
// 	for (int i = 0; i < 6; i++) {
// 		printf("[proc2] iteration %d at t=%us — sleeping 6s\n", i,
// 		       time_seconds());
// 		if (i == 1) {
// 			printf("[proc2] spawning proc3 dynamically...\n");
// 			process_spawn(_proc3, "proc3", LOW, false);
// 		}
// 		scheduler_sleep(6);
// 	}
// 	printf("[proc2] done, exiting\n");
// }
//
// /** @brief Acquire the shared mutex, hold it for 15s, then release it. */
// static void _mutex_holder()
// {
// 	_ensure_mutex_init();
// 	printf("[holder] acquiring mutex...\n");
// 	mutex_lock(&shared_mutex);
// 	printf("[holder] mutex acquired! holding for 15s (run `ps` to see waiter "
// 	       "BLOCKED)\n");
// 	scheduler_sleep(15);
// 	printf("[holder] releasing mutex\n");
// 	mutex_unlock(&shared_mutex);
// 	printf("[holder] done\n");
// }
//
// /** @brief Acquire the shared mutex, blocking while the holder owns it. */
// static void _mutex_waiter()
// {
// 	_ensure_mutex_init();
// 	printf("[waiter] trying to acquire mutex (will BLOCK if holder has it)...\n");
// 	mutex_lock(&shared_mutex);
// 	printf("[waiter] mutex acquired! (unblocked by holder's release)\n");
// 	mutex_unlock(&shared_mutex);
// 	printf("[waiter] done\n");
// }
//
/* --- ps command --- */

/**
 * @brief Printable name of a process state.
 *
 * @param s State to convert.
 * @return Pointer to its static name.
 */
static const char *_state_str(state s)
{
	switch (s) {
	case NEW:
		return "NEW";
	case RUNNING:
		return "RUN";
	case READY:
		return "READY";
	case SLEEPING:
		return "SLEEP";
	case BLOCKED:
		return "BLOCK";
	case TERMINATED:
		return "TERM";
	case ZOMBIE:
		return "ZOMBIE";
	default:
		return "?";
	}
}

/**
 * @brief Print one process table row.
 *
 * @param node Process table node of the process.
 * @param args Additional args, unused.
 * @return 0 (invariant).
 */
static int _pretty_print_process(clist_node_t *node, void *args)
{
	(void)args;
	process_t *proc = container_of(node, process_t, proc_node);
	process_t *parent = proc->parent;
	uint8_t ppid = parent ? parent->pid : 0;
	printf("%-5d %-5d %-10s %-7s\n", proc->pid, ppid, proc->name,
	       _state_str(proc->state));
	return 0;
}

/** @brief Print the process table. */
static void ps()
{
	printf("%-5s %-5s %-10s %-7s\n", "PID", "PPID", "CMD", "STATE");
	clist_for_each(process_table_clist(), _pretty_print_process, NULL);
}

/* --- Register all programs --- */

#define REGISTER_USER_APP(func) cmd_register_prog(#func, func, NORMAL, true)
#define REGISTER_BUILT_IN(func) cmd_register_builtin(#func, func)

void programs_init()
{
	REGISTER_USER_APP(sleep_call);
	REGISTER_USER_APP(fork_test);
	REGISTER_USER_APP(segfault_test);
	REGISTER_USER_APP(write_test);
	REGISTER_USER_APP(read_test);
	REGISTER_USER_APP(stream_test);

	REGISTER_BUILT_IN(ps);
}
