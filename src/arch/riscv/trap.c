#include "cpu.h"
#include "platform.h"
#include "trap.h"
#include "syscall.h"
#include "time.h"
#include "irq.h"
#include "minilib/stdint.h"
#include "minilib/stddef.h"
#include "minilib/stdio.h"

extern void enter_user(pt_regs_t *regs);
extern void trap_entry(void);

static inline long _get_stub_sstatus()
{
	long sstatus;
	__asm__ volatile("csrr %0, sstatus" : "=r"(sstatus));

	// Set the previous mode to user mode
	sstatus |= SSTATUS_SPP;
	// SPIE is set to 1 in kernel trap since MIE is set to 1
	sstatus |= SSTATUS_SPIE;

	return sstatus;
}

void enter_user_mode(void (*entry)(), uintptr_t ustack)
{
	// This structure is gonna be on kernel stack
	pt_regs_t regs = { 0 };

	long sstatus = _get_stub_sstatus();
	// Smoke regs entry from the user
	regs.sepc = (long)entry;
	regs.sstatus = sstatus;
	regs.sp = ustack;

	// Make the sp points to the stub frame
	// and then return from this frame to user mode
	enter_user(&regs);
};

/**
 * @brief Set the trap entry point called to treat the irq.
 *
 * TODO: This function might be unused after adding the S mode.
 *
 * @param entry Trap vector written in the mtvec register.
 *
 */
static inline void init_mtvec(void (*entry)())
{
	__asm__("csrw mtvec, %0" ::"r"(entry));
}

static inline void init_stvec(void (*entry)())
{
	__asm__ __volatile__("csrw stvec, %0" ::"r"(entry));
}

static inline void _stop()
{
	disable_s_irq();
	disable_m_irq();
	for (;;)
		hlt();
}

/**
 * @brief Pretty print panic function for trap handling.
 *
 * @param cause Cause register: m/scause register.
 * @param pc m/sepc register.
 * @param tval Trap value of the exception m/stval.
 */
static inline void _panic_print(long cause, long pc, long tval)
{
	if (cause & XCAUSE_IRQ_BIT) {
		cause &= XCAUSE_IRQ_BIT;
		printf("[PANIC/IRQ]: cause=%ld mepc=0x%lx mtval=0x%lx\n", cause,
		       pc, tval);
		;
	} else {
		printf("[PANIC/EXCEPTION]: cause=%ld mepc=0x%lx mtval=0x%lx\n",
		       cause, pc, tval);
	};
}

/**
 * @brief Machine level trap panic.
 */
static inline void _machine_trap_panic()
{
	long mtval;
	long mepc;
	long mcause;

	__asm__ __volatile__("csrr %0, mtval" : "=r"(mtval));
	__asm__ __volatile__("csrr %0, mepc" : "=r"(mepc));
	__asm__ __volatile__("csrr %0, mcause" : "=r"(mcause));

	printf("[PANIC]: FROM MACHINE \n");
	_panic_print(mcause, mepc, mtval);

	_stop();
}

/**
 * @brief Supervisor privilege kernel trap panic.
 *
 * @param t A pointer to the saved register of the trap.
 */
static inline void _kernel_trap_panic(pt_regs_t *t)
{
	long stval;
	__asm__ __volatile__("csrr %0, stval" : "=r"(stval));

	printf("[PANIC]: FROM KERNEL \n");
	_panic_print(t->scause, t->sepc, stval);
	_stop();
}

void init_trap_entries()
{
	init_mtvec(_machine_trap_panic);
	init_stvec(trap_entry);
}

void trap_handler(pt_regs_t *t)
{
	uint64_t scause = t->scause;
	long irq_flag = scause & XCAUSE_IRQ_BIT;

	if (irq_flag) {
		long irq_cause = scause & ~XCAUSE_IRQ_BIT;
		switch (irq_cause) {
		case S_IRQ_EXT:
			external_irq_handler();
			break;
		case S_IRQ_TMR:
			timer_irq_handler();
			break;
		}
	} else {
		switch (scause) {
		case ECALL_UMODE:
			printf("[Kernel/INFO]: Trap call from U mode \n");
			t->sepc += 4;
			t->a[0] = syscall_dispatch(t->a[7], t->a[0], t->a[1],
						   t->a[2]);
			break;

		default:
			_kernel_trap_panic(t);
			break;
		}
	}
}
