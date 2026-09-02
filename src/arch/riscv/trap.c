#include "platform.h"
#include "trap.h"
#include "syscall.h"
#include "time.h"
#include "irq.h"
#include "minilib/stdint.h"
#include "minilib/stdio.h"

extern void enter_user(pt_regs_t *regs);

static inline long _get_stub_machine_mstatus()
{
	long mstatus;
	__asm__ volatile("csrr %0, mstatus" : "=r"(mstatus));

	// Set the previous mode to user mode
	mstatus &= ~MSTATUS_MPP_MASK;
	mstatus |= (U << MSTATUS_MPP_SHIFT) & MSTATUS_MPP_MASK;
	// MPIE is set to 1 in kernel trap since MIE is set to 1
	mstatus |= MSTATUS_MPIE;

	return mstatus;
}

void enter_user_mode(void (*entry)(), uintptr_t ustack)
{
	// This structure is gonna be on kernel stack
	pt_regs_t regs = { 0 };

	long mstatus = _get_stub_machine_mstatus();
	// Smoke regs entry from the user
	regs.mepc = (long)entry;
	regs.mstatus = mstatus;
	regs.sp = ustack;

	// Make the sp points to the stub frame
	// and then return from this frame to user mode
	enter_user(&regs);
};

void init_trap_entry(void (*entry)())
{
	__asm__("csrw mtvec, %0" ::"r"(entry));
}

void trap_handler(pt_regs_t *t)
{
	uint64_t mcause = t->mcause;
	if (mcause & MCAUSE_IRQ_BIT) {
		switch (mcause & MCAUSE_IRQ_MASK) {
		case IRQ_M_EXT:
			external_irq_handler();
			break;
		case IRQ_M_TMR:
			timer_irq_handler();
			break;
		}
	} else {
		switch (mcause) {
		case ECALL_MMODE:
		case ECALL_UMODE:
			printf("[Kernel/INFO]: Trap call from U mode \n");
			t->mepc += 4;
			t->a[0] = syscall_dispatch(t->a[7], t->a[0], t->a[1],
						   t->a[2]);
			break;

		default:
			printf("[Kernel/ERROR]: cannot handle this ecall: %lu \n",
			       mcause);
			break;
		}
	}
}
