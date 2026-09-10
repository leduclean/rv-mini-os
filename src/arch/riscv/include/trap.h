/**
 * @file
 * @brief Trap entry point and register frame saved on a trap.
 */

#pragma once
#include "asm_defs.h"
#include "ldsym.h"
#include "minilib/stdint.h"

// Forward declaration
typedef struct process process_t;

#define USTACK_TOP (MAX_VA - 3 * PAGE_SIZE)
#define USTACK (MAX_VA - 2 * PAGE_SIZE)

/**
 * @brief Registers of the interrupted context, saved on the trap frame.
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
 * @brief Entrypoint to the user mode from the kernel.
 *
 * @notes This function should be used only for user programs.
 *
 * @param entry The user program entry point.
 */
void enter_user_mode(const process_t *proc);

/**
 * @brief Inits trap entries of M and S privileges modes.
 * @warning This function MUST be called during the kernel boot.
 *
 */
void init_trap_entries();

extern char trap_return[];

typedef void (*trap_return_fn)(unsigned long satp);

/**
 * @brief Get a pointer to the trap return virtual addr.
 *
 * @return Pointer to the function
 */
static inline trap_return_fn get_trap_return_va()
{
	return (trap_return_fn)(TRAMPOLINE + (trap_return - _trampoline_start));
}
