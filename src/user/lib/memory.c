#include "memory.h"
#include "minilib/stdint.h"

void pmp_allow_all()
{
	//NOTE: We use the whole memory for the process to enter user mode
	__asm__ __volatile__("csrw pmpaddr0, %0" ::"r"(~0UL));
	// NOTE: this mode is a stub to enter user mode and smoke test the user mode
	__asm__ __volatile__("csrw pmpcfg0, %0" ::"r"(PMP_A_NAPOT | PMP_R_BIT |
						      PMP_W_BIT | PMP_X_BIT));
}
