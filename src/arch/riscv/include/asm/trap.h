/**
 * @file
 * @brief Trap entry point and register frame saved on a trap.
 */

#pragma once
#include <stdint.h>

#include <asm/asm_defs.h>

#include <kernel/ldsym.h>

#include "kernel/vpages.h"

// Forward declaration
typedef struct process process_t;

#define USTACK_TOP (MAX_VA - 3 * PAGE_SIZE)
#define USTACK (MAX_VA - 2 * PAGE_SIZE)

/**
 * @brief Trap frame va of the thread @p pid.
 *
 * Below the main user stack, MAX_VA - 4 * PAGE_SIZE stays unmapped as a guard.
 */
#define THREAD_TRAPFRAME(pid) (MAX_VA - (4 + (unsigned long)(pid)) * PAGE_SIZE)

/**
 * @brief Registers of the interrupted context, saved on the trap frame.
 *
 * @note trampoline.S hardcodes these offsets, keep both in sync. The frame
 * is 32 slots so that its size keeps the stack pointer 16 bytes aligned. gp and tp are not saved yet, they become mandatory once user code runs.
 */
typedef struct pt_regs {
	uint64_t sp; ///< Stack pointer of the interrupted context.
	uint64_t ra;
	uint64_t a[8];
	uint64_t t[7];
	uint64_t s[12];
	uint64_t sepc; ///< Address the trap returns to.
	uint64_t scause; ///< Cause of the trap.
	uint64_t sstatus; ///< Status of the interrupted context, SPP included.
	uint64_t satp; ///< User satp.
} pt_regs_t;

/* trampoline.S hardcodes these offsets, keep both in sync. */
_Static_assert(sizeof(pt_regs_t) == 33 * 8, "trampoline.S frame size stale");
_Static_assert(__builtin_offsetof(pt_regs_t, a) == 2 * 8,
	       "trampoline.S offsets stale");
_Static_assert(__builtin_offsetof(pt_regs_t, t) == 10 * 8,
	       "trampoline.S offsets stale");
_Static_assert(__builtin_offsetof(pt_regs_t, s) == 17 * 8,
	       "trampoline.S offsets stale");
_Static_assert(__builtin_offsetof(pt_regs_t, sepc) == 29 * 8,
	       "trampoline.S offsets stale");
_Static_assert(__builtin_offsetof(pt_regs_t, sstatus) == 31 * 8,
	       "trampoline.S offsets stale");

typedef struct tframe {
	pt_regs_t saved_regs;
	unsigned long kstack;
	unsigned long ksatp;
} tframe_t;

_Static_assert(__builtin_offsetof(tframe_t, kstack) == 33 * 8,
	       "trampoline.S offsets stale");
_Static_assert(__builtin_offsetof(tframe_t, ksatp) == 34 * 8,
	       "trampoline.S offsets stale");

/**
 * @brief Bind the trap frame @p t to its owner.
 *
 * Sets only what depends on the owner: user sstatus, user satp, kstack and
 * kernel satp. The saved registers are left untouched, so it can be called
 * on a fresh frame (page_alloc zeroes it) as well as on a forked copy.
 *
 * @param t The trap frame to init.
 * @param root The owner root table, for satp.
 * @param kstack_top The top of the owner kstack.
 */
void tframe_init(tframe_t *t, pte_t *root, unsigned long kstack_top);

/**
 * @brief Set the user stack pointer, aligned down on 16 bytes.
 *
 * @param t The trap frame.
 * @param ustack Top of the user stack.
 */
static inline void tframe_start(tframe_t *t, void *entry, void *ustack)
{
	t->saved_regs.sp = (unsigned long)ustack;
	t->saved_regs.sepc = (unsigned long)entry;
}

/**
 * @brief Set the argument register a[@p idx].
 *
 * @param t The trap frame.
 * @param idx Argument index, in [0, 8[.
 * @param value Value of the argument.
 */
static inline void tframe_set_arg(tframe_t *t, int idx, unsigned long value)
{
	t->saved_regs.a[idx] = value;
}

/**
 * @brief Set the value returned to user mode, ie a0.
 *
 * @param t The trap frame.
 * @param value Returned value.
 */
static inline void tframe_set_return_val(tframe_t *t, unsigned long value)
{
	t->saved_regs.a[0] = value;
}

/**
 * @brief Entrypoint to the user mode from the kernel.
 *
 * @notes This function should be used only for user programs.
 */
void trap_return_to_user(void);

/**
 * @brief Inits trap entries of M and S privileges modes.
 * @warning This function MUST be called during the kernel boot.
 *
 */
void trap_init(void);

extern char trap_return[];

typedef void (*trap_return_fn)(unsigned long satp, unsigned long curr_tframe);

/**
 * @brief Get a pointer to the trap return virtual addr.
 *
 * @return Pointer to the function
 */
static inline trap_return_fn trap_return_va(void)
{
	return (trap_return_fn)(TRAMPOLINE + (trap_return - _trampoline_start));
}
