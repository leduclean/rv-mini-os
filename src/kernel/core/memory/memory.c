#include <asm/csr.h>

#include <kernel/memory.h>

void pmp_allow_all()
{
	//NOTE: We use the whole memory for the process to enter user mode
	csr_write(pmpaddr0, ~0UL);
	// NOTE: this mode is a stub to enter user mode and smoke test the user mode
	csr_write(pmpcfg0, PMP_A_NAPOT | PMP_R_BIT | PMP_W_BIT | PMP_X_BIT);
}
