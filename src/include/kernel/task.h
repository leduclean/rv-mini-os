/**
 * @file
 * @brief Task control block and task table handling.
 */

#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lib/clist.h>

#include <arch/abi.h>
#include <arch/trap.h>
#include <kernel/fs.h>
#include <kernel/vpages.h>
#include <kernel/waitqueue.h>

#define KSTACK_SIZE 512 ///< Size of a task stack, in 64 bits words.
#define NOFILE 16 /**< Number of file descriptor */

/** @brief Lifecycle states of a task. */
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
	uint64_t sp; ///> Stack pointer of the task.
	uint64_t s[12]; ///> Callee saved registry.
} ctx_t;

/* ctxt.S hardcodes these offsets, keep both in sync. */
static_assert(offsetof(ctx_t, s) == 2 * 8, "ctxt.S offsets stale");

/** @brief Scheduling priorities, sorted decremental (HIGH is the highest). */
typedef enum { HIGH = 0, NORMAL, LOW, IDLE, PRIORITY_COUNT } priority;

/** @brief Task control block. */
typedef struct task {
	uint8_t pid; ///< Task identifier.
	uint8_t tgid; ///< Thread group id.
	void (*code)(void); ///< Kernel task entry, read by ktask_launcher.
	char name[MAXNAME]; ///< Task name, null terminated.
	state state; ///< Current lifecycle state.
	ctx_t ctx; ///< Registers saved on a context switch.
	uint64_t kstack[KSTACK_SIZE]; ///< Task stack.
	pte_t *root_ptable; ///< Task Root Page table.
	bool user; ///< User mode Task flag (fixed at creation).
	tframe_t *
		tframe_pa; //< The User trap frame used to save the user context.
	unsigned long
		tframe_va; //< The User virtual trap frame used to go back to user.

	clist_node_t task_node; ///< Task table node.
	clist_node_t ready_node; ///< Scheduler ready queue node.
	clist_node_t
		wait_node; ///< Wait queue node, used to put the task in a zombie, IO or Mutex wait state.
	clist_node_t sleep_node; ///< Sleeping queue node.
	uint64_t wake_up_time; ///< Wake up date, in secondes since boot.

	task_t *parent; ///< Parent task, NULL if orphan.
	clist_node_t zombies; ///< Terminated children waiting to be reaped.
	wait_queue_t zombie_wq; ///< Queue for blocked thread on this thread.
	int8_t exit_code; ///< Code to identify what made our task exit.

	priority priority; ///< Scheduling priority class.

	struct file *fdtable[NOFILE]; //< Task specific file descriptor table.
} task_t;

/**
 * @brief Make a task the active one, demoting the previous one to READY.
 *
 * @param next Task to make active.
 */
void task_switch_active(task_t *next);

/**
 * @brief Get the clist holding every living task.
 *
 * @return Head sentinel of the task table clist.
 */
const clist_node_t *task_table_clist(void);

/**
 * @brief Get the running task.
 *
 * @return Pointer to the active task.
 */
task_t *task_active(void);

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
 * @brief Set a task in sleeping state with a timer.
 *
 * @param task Task to put to sleep.
 * @param delay Sleeping duration, in secondes.
 */
void task_sleep(task_t *task, uint32_t delay);

/**
 * @brief Set a task to blocked state, waiting for an IO irq.
 *
 * @param task Task to block.
 */
void task_block(task_t *task);

/**
 * @brief Switch a sleeping or blocked task back to running.
 *
 * @note It is also removed from every blocking queue it was in.
 *
 * @param task Task to wake up.
 */
void task_wake(task_t *task);

/**
 * @brief Zombify a task, putting it in the zombies queue of its parent.
 *
 * @note The parent is woken up if it was waiting for a child.
 * @note Use this when delayed destroy is needed.
 * @warning Callling this function on an orphan task will led to panic.
 *
 * @param task Task to zombify.
 *
 */
void task_zombify(task_t *task, int exit_code);

/**
 * @brief Clean a task up, removing it from all queues and from memory.
 *
 * @warning Calling this function on the current task will led to 
 * panic. If you want to do this, you should zombify the task via @task_zombify instead.
 *
 * @param task Task to clean up.
 */
void task_destroy(task_t *task);

/**
 * @brief Reap a zombie task, called by its parent.
 *
 * @param task Zombie task to reap.
 */
static inline void task_reap(task_t *task)
{
	if (task->state != ZOMBIE) {
		return;
	}

	task_destroy(task);
}

/** @brief Idle task, runs whenever no other task is ready. */
[[noreturn]] void task_idle(void);

/**
 * @brief Init the task table, the scheduler queues and the idle task.
 *
 * @retval 0 Success.
 * @retval -Exxx See task_kthread().
 * */
int task_init(void);

/**
 * @brief Spawn a kthread and admit it in the scheduler.
 *
 * @param code Entry point of the task.
 * @param name Name of the task, truncated to MAXNAME - 1 chars.
 * @param prior Priority of the task.
 * @param user User mode task flag.
 * @return The spawned task.
 * @retval ERR_PTR(-EAGAIN) Task table full.
 * @retval ERR_PTR(-ENOMEM) Out of memory.
 */
task_t *task_kthread(void code(void), const char *name, priority prior);

/**
 * @brief Creates the first userland task.
 *
 * @param user_elf Elf file of the executed user code.
 * @param name Task name.
 * @param prior Priority of this userland program.
 * @return The spawned task.
 * @retval ERR_PTR(-Exxx) See task_kthread(), task_exec().
 */
task_t *task_start_userland(const uintptr_t elf, const char *name,
				  priority prior);

/**
 * @brief Clone a task with granularity on which to copy or not.
 *
 * @param parent The parent to copy.
 * @param entry The entry point of the child, NEEDED if @CLONE_VM is set.
 * @param ustack A new stack, NEEDED if @CLONE_VM is set.
 * @param flags  Flags to specify which element to copy. 
 * @return The child.
 * @retval ERR_PTR(-EINVAL) CLONE_VM without @p entry or @p stack.
 * @retval ERR_PTR(-EAGAIN) Task table full.
 * @retval ERR_PTR(-ENOMEM) Out of memory.
 */
task_t *task_clone(task_t *parent, void *entry, void *args,
			 void *stack, unsigned long flags);

/**
 * @brief Duplicate a task state making a copy of it execution state.
 *
 * @warning This function is syscall only, you can't call it from 
 * a non user parent.
 *
 * @return The child.
 * @retval ERR_PTR(-Exxx) See task_clone().
 */
task_t *task_fork(task_t *parent);

/**
 * @brief Replaces a current running task image with a new code, new stack and a new
 * memory map.
 *
 * @param p A pointer to the task to replace.
 * @param elf Elf file of a newly executed program.
 * @param argv User argument to execute the elf file.
 * @retval 0 Success.
 * @retval -ENOEXEC Bad elf.
 * @retval -E2BIG More than EXEC_MAX_ARGS arguments.
 * @retval -EFAULT Bad user address.
 * @retval -ENAMETOOLONG An argument exceeds MAXNAME.
 * @retval -ENOMEM Out of memory.
 */
int task_exec(task_t *p, const uintptr_t elf, char *const argv[]);

/**
 * @brief Attribute a in-task file descriptor number to a file descriptor.
 *
 * @param p Pointer to the task to attribute fd.
 * @param f Pointer to the file descriptor structure.
 * @return The fd number.
 * @retval -EMFILE Fd table full.
 */
int task_give_fd(task_t *p, struct file *f);

/**
 * @brief Get the file descriptor associated with the task file number.
 *
 * @param p A pointer to the task owning the fd.
 * @param fd The file descriptor associated structure or NULL if doesn't exists.
 */
struct file *task_get_fd(const task_t *p, int fd);

/**
 * @brief Release the file descriptor owned by the task.
 *
 * @param fd The file descriptor.
 * @retval 0 Success.
 * @retval -EBADF @p fd is not open.
 */
int task_release_fd(task_t *p, int fd);

/**
 * @brief Map the console in the task file descriptor.
 *
 * @param p A pointer to the first task mapped.
 * @retval 0 Success.
 * @retval -Exxx See fs_mknod(), file_open().

 * @pre fs_init()
 should be called before this function.
 */
int task_map_console(task_t *p);
