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
 * @brief Map a user process page table.
 *
 * @param p A pointer to the process to map.
 * @return [TODO: error codes]
 */
int mmap_uprocess(process_t *p);

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
