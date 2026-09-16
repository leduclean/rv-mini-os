#include <lib/stdio.h>

#include <asm/asm_defs.h>
#include <asm/csr.h>
#include <asm/platform.h>

#include <kernel/ldsym.h>
#include <kernel/mmap.h>
#include <kernel/pages.h>
#include <kernel/process.h>
#include <kernel/vpages.h>

#include "asm/trap.h"

static pte_t *kroot;

#define SATP_SV39_MODE 8UL
#define SATP_MODE_SHIFT 60
#define SATP_MODE_MASK (0xfUL << SATP_MODE_SHIFT)

unsigned long mmap_satp(void *pt)
{
	return ((SATP_SV39_MODE << SATP_MODE_SHIFT) | PAGE_NUMBER(pt));
}

unsigned long mmap_kernel_satp(void)
{
	return mmap_satp(kroot);
}

void mmap_switch_to_kernel(void)
{
	csr_write(satp, mmap_kernel_satp());
	mmap_update_tlb();
}

int mmap_kernel(void)
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

int mmap_uspace(process_t *p)
{
	int res;
	if (!p->user) {
		res = -1;
		goto out;
	}

	pte_t *root = page_alloc();
	if (!root) {
		res = -1;
		goto out;
	}

	tframe_t *t = page_alloc();
	if (!t) {
		res = -1;
		goto err_free_tree;
	}

	res = vpage_map(root, (void *)TRAPFRAME, t, PTE_R | PTE_W);
	if (res < 0) {
		// t failed to get mapped so no in the tree yet
		goto err_free_tframe;
	}

	p->root_ptable = root;
	p->tframe_pa = t;

	return 0;

err_free_tframe:
	page_put(t);
err_free_tree:
	vpage_tree_free(root);
out:
	return res;
}

int mmap_uimage(process_t *p)
{
	int res;

	res = mmap_uspace(p);
	if (res < 0) {
		return res;
	}

	pte_t *root = p->root_ptable;

	// The whole code is identity mapped for now.
	res = vpage_map_user_range(root, _user_start, _user_start,
				   _user_end - _user_start,
				   PTE_X | PTE_R | PTE_G);
	if (res < 0) {
		goto err_free_tree;
	}

	// This is not U page because we will go back to user page
	// in S mode before the sret returns to user mode.
	res = vpage_map_range(root, (void *)TRAMPOLINE, _trampoline_start,
			      _trampoline_end - _trampoline_start,
			      PTE_X | PTE_R | PTE_G);
	if (res < 0) {
		goto err_free_tree;
	}

	// Stack lives within the image so it's allocated
	// during a image mapping.
	void *stack = page_alloc();
	if (!stack) {
		res = -1;
		goto err_free_tree;
	}

	res = vpage_map_user(root, (void *)USTACK_TOP, stack, PTE_R | PTE_W);
	if (res < 0) {
		page_put(stack);
		goto err_free_tree;
	}

	return 0;

err_free_tree:
	vpage_tree_free(root);
	p->root_ptable = NULL;
	p->tframe_pa = NULL;
	return res;
}
