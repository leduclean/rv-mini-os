#include "cpu.h"
#include "scheduler.h"
#include "user_entry.h"
#include "asm_defs.h"
#include "mmap.h"
#include "platform.h"
#include "trap.h"
#include "process.h"
#include "syscall.h"
#include "time.h"
#include "irq.h"
#include "minilib/stddef.h"
#include "minilib/stdio.h"
#include "csr.h"
#include "vpages.h"

static inline unsigned long _get_user_sstatus()
{
	unsigned long val = csr_read(sstatus);

	// Set the previous mode to user mode: SPP=0 -> sret returns to U,
	// SPP=1 would return to S.
	val &= ~SSTATUS_SPP;
	// SPIE is set to 1 in kernel trap since MIE is set to 1
	val |= SSTATUS_SPIE;

	return val;
}

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
	csr_write(mtvec, entry);
}

static inline void init_stvec(void (*entry)())
{
	csr_write(stvec, entry);
}

/**
 * @brief Pretty print panic function for trap handling.
 *
 * @param cause Cause register: m/scause register.
 * @param pc m/sepc register.
 * @param tval Trap value of the exception m/stval.
 */
static inline void _panic_print(unsigned long cause, unsigned long pc,
				unsigned long tval)
{
	if (cause & XCAUSE_IRQ_BIT) {
		cause &= XCAUSE_IRQ_BIT;
		printf("[PANIC/IRQ]: cause=%ld epc=0x%lx tval=0x%lx\n", cause,
		       pc, tval);
		;
	} else {
		printf("[PANIC/EXCEPTION]: cause=%ld epc=0x%lx tval=0x%lx\n",
		       cause, pc, tval);
	};
}

/**
 * @brief Machine level trap panic.
 */
static inline void _machine_trap_panic()
{
	unsigned long mtval = csr_read(mtval);
	unsigned long mepc = csr_read(mepc);
	unsigned long mcause = csr_read(mcause);

	printf("[PANIC]: FROM MACHINE \n");
	_panic_print(mcause, mepc, mtval);

	panic("unhandled machine trap");
}

/**
 * @brief Supervisor privilege kernel trap panic.
 *
 * @param t A pointer to the saved register of the trap.
 */
static inline void _kernel_trap_panic(unsigned long scause, unsigned long sepc,
				      unsigned long stval)
{
	printf("[PANIC]: FROM KERNEL \n");
	_panic_print(scause, sepc, stval);
	panic("unhandled kernel trap");
}

static inline void _handler_async_irq(unsigned long irq_cause)
{
	switch (irq_cause) {
	case S_IRQ_EXT:
		external_irq_handler();
		break;
	case S_IRQ_TMR:
		timer_irq_handler();
		break;
	}
}

/**
 * @brief Asm entry point called when trapping from kernel mode.
 */
void kerneltrap()
{
	unsigned long scause = csr_read(scause);
	volatile unsigned long sstatus = csr_read(sstatus);
	volatile unsigned long sepc = csr_read(sepc);

	long irq_flag = scause & XCAUSE_IRQ_BIT;
	if (irq_flag) {
		unsigned long irq_cause = scause & ~XCAUSE_IRQ_BIT;
		_handler_async_irq(irq_cause);

	} else {
		//TODO: We do not handle exception from kernel to kernel.
		unsigned long stval = csr_read(stval);
		_kernel_trap_panic(scause, sepc, stval);
	};

	// Restore initial csr for trap in trap handling
	csr_write(sstatus, sstatus);
	csr_write(sepc, sepc);
}

/**
 * @brief Asm entry point called when trapping from user mode.
 */
unsigned long usertrap()
{
	process_t *p = get_active();
	pt_regs_t *t = &p->tframe_pa->saved_regs;

	unsigned long scause = t->scause;
	unsigned long stval = csr_read(stval);
	long irq_flag = scause & XCAUSE_IRQ_BIT;

	if (irq_flag) {
		unsigned long irq_cause = scause & ~XCAUSE_IRQ_BIT;
		_handler_async_irq(irq_cause);
	} else {
		switch (scause) {
		case ECALL_UMODE:
			printf("[Kernel/INFO]: Trap call from U mode \n");
			t->sepc += 4;
			t->a[0] = syscall_dispatch(t->a[7], t->a[0], t->a[1],
						   t->a[2]);
			break;
		case STORE_PAGE_FAULT: {
			// TODO: drop once CoW is trusted, this fires per page.
			printf("[Kernel/INFO]: CoW fault on 0x%lx (pid %d)\n",
			       stval, p->pid);
			if (vpage_handle_cow(p->root_ptable, (void *)stval) ==
			    0) {
				break;
			}
		}
			__attribute__((fallthrough));
		case LOAD_PAGE_FAULT:
		case INSTRUCTION_PAGE_FAULT: {
			printf("[Kernel]: SEGFAULT - va 0x%lx, epc 0x%lx, cause %ld (pid %d)\n",
			       stval, t->sepc, scause, p->pid);
			scheduler_terminate(-1);
			break;
		}

		default: {
			_kernel_trap_panic(scause, t->sepc, stval);
			break;
		}
		}
	}
	return t->satp;
}

extern void kernelvec();

void init_trap_entries()
{
	init_mtvec(_machine_trap_panic);
	// Kernel vec when starting the OS
	init_stvec(kernelvec);
}

void enter_user_mode()
{
	process_t *p = get_active();
	tframe_t *t = p->tframe_pa;

	// Wanted initial user state
	t->saved_regs.sp = USTACK;

	// Set the starting function to the app code
	t->saved_regs.a[0] = (unsigned long)p->code;
	t->saved_regs.sepc = (unsigned long)user_entry_point;

	t->saved_regs.sstatus = _get_user_sstatus();
	t->saved_regs.satp = get_satp(p->root_ptable);

	// Kernel state
	t->ksatp = get_kernel_satp();
	t->kstack = (unsigned long)&p->kstack[KSTACK_SIZE];

	get_trap_return_va()(t->saved_regs.satp);
};
