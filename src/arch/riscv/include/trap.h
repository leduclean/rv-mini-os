/**
 * @file
 * @brief Trap entry point and register frame saved on a trap.
 */

#pragma once
#include "minilib/stdint.h"

/**
 * @brief Registers of the interrupted context, saved on the kernel stack.
 *
 * @note trampoline.S hardcodes these offsets, keep both in sync. The frame
 * is 32 slots so that its size keeps the stack pointer 16 bytes aligned.
 * gp and tp are not saved yet, they become mandatory once user code runs.
 */
typedef struct pt_regs {
	uint64_t sp; ///< Stack pointer of the interrupted context.
	uint64_t ra;
	uint64_t a[8];
	uint64_t t[7];
	uint64_t s[12];
	uint64_t mepc; ///< Address the trap returns to.
	uint64_t mcause; ///< Cause of the trap.
	uint64_t mstatus; ///< Status of the interrupted context, MPP included.
} pt_regs_t;

/* trampoline.S hardcodes these offsets, keep both in sync. */
_Static_assert(sizeof(pt_regs_t) == 32 * 8, "trampoline.S frame size stale");
_Static_assert(__builtin_offsetof(pt_regs_t, a) == 2 * 8,
	       "trampoline.S offsets stale");
_Static_assert(__builtin_offsetof(pt_regs_t, t) == 10 * 8,
	       "trampoline.S offsets stale");
_Static_assert(__builtin_offsetof(pt_regs_t, s) == 17 * 8,
	       "trampoline.S offsets stale");
_Static_assert(__builtin_offsetof(pt_regs_t, mepc) == 29 * 8,
	       "trampoline.S offsets stale");
_Static_assert(__builtin_offsetof(pt_regs_t, mstatus) == 31 * 8,
	       "trampoline.S offsets stale");

/**
 * @brief Entrypoint to the user mode from the kernel.
 *
 * @notes This function should be used only for user programs.
 *
 * @param entry The user program entry point.
 * @param ustack The stack of the user program.
 */
void enter_user_mode(void (*entry)(), uintptr_t ustack);

/**
 * @brief Set the trap entry point called to treat the irq.
 *
 * @param entry Trap vector written in the mtvec register.
 */
void init_trap_entry(void (*entry)());

/**
 * @brief Trap handler called by the trampoline, on the kernel stack.
 *
 * @note Dispatches to the interrupt handlers or to the syscall dispatch,
 * depending on the cause of the trap.
 *
 * @param regs Registers of the interrupted context. Writing to them changes
 * what the trampoline restores before returning.
 */
void trap_handler(pt_regs_t *regs);
