#pragma once
/**
 * @file
 * @brief This file give the interface for the sv39 virtual paging.
 */

#include <stddef.h>

#include <kernel/process.h>

extern char _trampoline_start[], _trampoline_end[];

/**
 * @brief Map the kernel page table root. 
 * 
 * @note The corresponding satp is accessible via @ref mmap_kernel_satp.
 * @return [TODO: error codes]
 */
int mmap_kernel(void);

/**
 * @brief Allocate and map what each user process owns (root pd and a trapframe).
 *
 * @note fork stops here. The tree owns the trapframe too:
 * vpage_tree_free(p->root_ptable) frees everything.
 *
 * @param p A pointer to the owner process.
 * @return 0 if sucess, error code < 0 on failure.
 */
int mmap_uspace(process_t *p);

/**
 * @brief mmap_uspace, code, trampoline and stack. The no parent case.
 *
 * @note The tree owns the trapframe on sucess.
 *
 * @warning On failure nothing stays allocated, you don't need 
 * to free the tree by yourself.
 *
 * @param p A pointer to the owner process.
 * @return 0 if sucess, error code < 0 on failure.
 */
int mmap_uimage(process_t *p);

/**
 * @brief Get the kernel satp. 
 *
 * @warning @ref mmap_kernel must have been called before.
 * or you get a random state.
 */
unsigned long mmap_kernel_satp(void);

/**
 * @brief Switch to the kernel ptable updating the satp reg.
 */
void mmap_switch_to_kernel(void);
/**
 * @brief Get satp encoded value from a physical page pointer
 *
 * @param pt A physical page pointer.
 */
unsigned long mmap_satp(void *pt);

/**
 * @brief Helper to update the tlb cache.
 */
static inline void mmap_update_tlb(void)
{
	__asm__ volatile("sfence.vma zero, zero" ::: "memory");
}
