/**
 * @file
 * @brief Process control block and process table handling.
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

#define MAXNAME STACK_ALIGN ///< Size of the process name buffer, in bytes.
#define KSTACK_SIZE 512 ///< Size of a process stack, in 64 bits words.
#define NOFILE 16 /**< Number of file descriptor */

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
static_assert(offsetof(ctx_t, s) == 2 * 8, "ctxt.S offsets stale");

/** @brief Scheduling priorities, sorted decremental (HIGH is the highest). */
typedef enum { HIGH = 0, NORMAL, LOW, IDLE, PRIORITY_COUNT } priority;

/** @brief Process control block. */
typedef struct process {
	uint8_t pid; ///< Process identifier.
	uint8_t tgid; ///< Thread group id.
	void (*code)(void); ///< Kernel proc entry, read by _kproc_launcher.
	char name[MAXNAME]; ///< Process name, null terminated.
	state state; ///< Current lifecycle state.
	ctx_t ctx; ///< Registers saved on a context switch.
	uint64_t kstack[KSTACK_SIZE]; ///< Process stack.
	pte_t *root_ptable; ///< Process Root Page table.
	bool user; ///< User mode Process flag (fixed at creation).
	tframe_t *
		tframe_pa; //< The User trap frame used to save the user context.
	unsigned long
		tframe_va; //< The User virtual trap frame used to go back to user.

	clist_node_t proc_node; ///< Process table node.
	clist_node_t ready_node; ///< Scheduler ready queue node.
	clist_node_t
		wait_node; ///< Wait queue node, used to put the process in a zombie, IO or Mutex wait state.
	clist_node_t sleep_node; ///< Sleeping queue node.
	uint64_t wake_up_time; ///< Wake up date, in secondes since boot.

	process_t *parent; ///< Parent process, NULL if orphan.
	clist_node_t zombies; ///< Terminated children waiting to be reaped.
	wait_queue_t zombie_wq; ///< Queue for blocked thread on this thread.
	int8_t exit_code; ///< Code to identify what made our process exit.

	priority priority; ///< Scheduling priority class.

	struct file *fdtable[NOFILE]; //< Task specific file descriptor table.
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
const clist_node_t *process_table_clist(void);

/**
 * @brief Get the running process.
 *
 * @return Pointer to the active process.
 */
process_t *process_active(void);

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
 * @brief Zombify a process, putting it in the zombies queue of its parent.
 *
 * @note The parent is woken up if it was waiting for a child.
 * @note Use this when delayed destroy is needed.
 * @warning Callling this function on an orphan proc will led to panic.
 *
 * @param proc Process to zombify.
 *
 */
void process_zombify(process_t *proc, int exit_code);

/**
 * @brief Clean a process up, removing it from all queues and from memory.
 *
 * @warning Calling this function on the current task will led to 
 * panic. If you want to do this, you should zombify the task via @process_zombify instead.
 *
 * @param proc Process to clean up.
 */
void process_destroy(process_t *proc);

/**
 * @brief Reap a zombie process, called by its parent.
 *
 * @param proc Zombie process to reap.
 */
static inline void process_reap(process_t *proc)
{
	if (proc->state != ZOMBIE) {
		return;
	}

	process_destroy(proc);
}

/** @brief Idle process, runs whenever no other process is ready. */
[[noreturn]] void process_idle(void);

/**
 * @brief Init the process table, the scheduler queues and the idle process.
 *
 * @retval 0 Success.
 * @retval -Exxx See process_kthread().
 * */
int process_init(void);

/**
 * @brief Spawn a kthread and admit it in the scheduler.
 *
 * @param code Entry point of the process.
 * @param name Name of the process, truncated to MAXNAME - 1 chars.
 * @param prior Priority of the process.
 * @param user User mode process flag.
 * @return The spawned process.
 * @retval ERR_PTR(-EAGAIN) Process table full.
 * @retval ERR_PTR(-ENOMEM) Out of memory.
 */
process_t *process_kthread(void code(void), const char *name, priority prior);

/**
 * @brief Creates the first userland task.
 *
 * @param user_elf Elf file of the executed user code.
 * @param name Task name.
 * @param prior Priority of this userland program.
 * @return The spawned task.
 * @retval ERR_PTR(-Exxx) See process_kthread(), process_exec().
 */
process_t *process_start_userland(const uintptr_t elf, const char *name,
				  priority prior);

/**
 * @brief Clone a proccess with granularity on which to copy or not.
 *
 * @param parent The parent to copy.
 * @param entry The entry point of the child, NEEDED if @CLONE_VM is set.
 * @param ustack A new stack, NEEDED if @CLONE_VM is set.
 * @param flags  Flags to specify which element to copy. 
 * @return The child.
 * @retval ERR_PTR(-EINVAL) CLONE_VM without @p entry or @p stack.
 * @retval ERR_PTR(-EAGAIN) Process table full.
 * @retval ERR_PTR(-ENOMEM) Out of memory.
 */
process_t *process_clone(process_t *parent, void *entry, void *args,
			 void *stack, unsigned long flags);

/**
 * @brief Duplicate a process state making a copy of it execution state.
 *
 * @warning This function is syscall only, you can't call it from 
 * a non user parent.
 *
 * @return The child.
 * @retval ERR_PTR(-Exxx) See process_clone().
 */
process_t *process_fork(process_t *parent);

/**
 * @brief Replaces a current running process image with a new code, new stack and a new
 * memory map.
 *
 * @param p A pointer to the process to replace.
 * @param elf Elf file of a newly executed program.
 * @param argv User argument to execute the elf file.
 * @retval 0 Success.
 * @retval -ENOEXEC Bad elf.
 * @retval -E2BIG More than EXEC_MAX_ARGS arguments.
 * @retval -EFAULT Bad user address.
 * @retval -ENAMETOOLONG An argument exceeds MAXNAME.
 * @retval -ENOMEM Out of memory.
 */
int process_exec(process_t *p, const uintptr_t elf, char *const argv[]);

/**
 * @brief Attribute a in-task file descriptor number to a file descriptor.
 *
 * @param p Pointer to the task to attribute fd.
 * @param f Pointer to the file descriptor structure.
 * @return The fd number.
 * @retval -EMFILE Fd table full.
 */
int process_give_fd(process_t *p, struct file *f);

/**
 * @brief Get the file descriptor associated with the task file number.
 *
 * @param p A pointer to the task owning the fd.
 * @param fd The file descriptor associated structure or NULL if doesn't exists.
 */
struct file *process_get_fd(const process_t *p, int fd);

/**
 * @brief Release the file descriptor owned by the task.
 *
 * @param fd The file descriptor.
 * @retval 0 Success.
 * @retval -EBADF @p fd is not open.
 */
int process_release_fd(process_t *p, int fd);

/**
 * @brief Map the console in the process file descriptor.
 *
 * @param p A pointer to the first task mapped.
 * @retval 0 Success.
 * @retval -Exxx See fs_mknod(), file_open().

 * @pre fs_init()
 should be called before this function.
 */
int process_map_console(process_t *p);
