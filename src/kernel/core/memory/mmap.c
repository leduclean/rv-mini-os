#include "mmap.h"

#include <stdio.h>

#include "asm_defs.h"
#include "csr.h"
#include "ldsym.h"
#include "pages.h"
#include "platform.h"
#include "process.h"
#include "vpages.h"

static pte_t *kroot;

#define SATP_SV39_MODE 8UL
#define SATP_MODE_SHIFT 60
#define SATP_MODE_MASK (0xfUL << SATP_MODE_SHIFT)

unsigned long mmap_satp(void *pt)
{
	return ((SATP_SV39_MODE << SATP_MODE_SHIFT) | PAGE_NUMBER(pt));
}

unsigned long mmap_kernel_satp()
{
	return mmap_satp(kroot);
}

void mmap_switch_to_kernel()
{
	csr_write(satp, mmap_kernel_satp());
	mmap_update_tlb();
}

int mmap_kernel()
{
	int res;
	kroot = page_alloc();
	if (!kroot) {
		return -1;
	}

	res = vpage_map_range(kroot, (void *)_ram_start, (void *)_ram_start,
			      (size_t)_heap_end - (size_t)_ram_start,
			      PTE_R | PTE_W | PTE_X);
	if (res < 0) {
		goto err_free_root;
	}

	res = vpage_map_range(kroot, (void *)TRAMPOLINE, _trampoline_start,
			      _trampoline_end - _trampoline_start,
			      PTE_X | PTE_R);
	if (res < 0) {
		goto err_free_root;
	}

	res = vpage_map_range(kroot, (void *)UART_BASE, (void *)UART_BASE,
			      PAGE_SIZE, PTE_R | PTE_W);
	if (res < 0) {
		goto err_free_root;
	}

	res = vpage_map_range(kroot, (void *)BOCHS_DISPLAY_BASE_ADDRESS,
			      (void *)BOCHS_DISPLAY_BASE_ADDRESS, DISPLAY_BYTES,
			      PTE_R | PTE_W);
	if (res < 0) {
		goto err_free_root;
	}

	res = vpage_map_range(kroot, (void *)PLIC_MMIO_BASE,
			      (void *)PLIC_MMIO_BASE, PLIC_MMIO_SIZE,
			      PTE_R | PTE_W);
	if (res < 0) {
		goto err_free_root;
	}

	// Alow Sv39 and write the kroot PPN to satp.
	csr_write(satp, mmap_satp(kroot));
	mmap_update_tlb();

	return 0;

err_free_root:
	page_put(kroot);
	return res;
}

int mmap_uprocess(process_t *p)
{
	printf("[INFO]: mapping user process %s \n", p->name);

	if (!p->user) {
		return 0;
	}

	pte_t *root = p->root_ptable;
	int res;

	// The whole code is identity mapped for now.
	res = vpage_map_user_range(root, _user_start, _user_start,
				   _user_end - _user_start,
				   PTE_X | PTE_R | PTE_G);
	if (res < 0) {
		return res;
	}

	void *stack = page_alloc();
	if (!stack) {
		return -1;
	}
	res = vpage_map_user(root, (void *)USTACK_TOP, stack, PTE_R | PTE_W);
	if (res < 0) {
		goto err_free_stack;
	}
	p->ustack_pa = stack;

	void *tframe = page_alloc();
	if (!tframe) {
		res = -1;
		goto err_free_stack;
	}
	res = vpage_map(root, (void *)TRAPFRAME, tframe, PTE_R | PTE_W);
	if (res < 0) {
		goto err_free_tframe;
	}
	p->tframe_pa = tframe;

	// This is not U page because we will go back to user page
	// in S mode before the sret returns to user mode.
	res = vpage_map_range(root, (void *)TRAMPOLINE, _trampoline_start,
			      _trampoline_end - _trampoline_start,
			      PTE_X | PTE_R | PTE_G);
	if (res < 0) {
		goto err_free_tframe;
	}

	printf("[INFO]: process %s mapped \n", p->name);
	return 0;

err_free_tframe:
	page_put(tframe);
err_free_stack:
	page_put(stack);
	return res;
}
