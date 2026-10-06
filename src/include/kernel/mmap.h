#pragma once
/**
 * @file
 * @brief This file give the interface for the sv39 virtual paging.
 */

#include <stddef.h>

#include <kernel/process.h>

extern char _trampoline_start[], _trampoline_end[];

/**
 * @brief Allocate the kernel page table tree and switch to kernel page table.
 * 
 * @note The corresponding satp is accessible via @ref mmap_kernel_satp.
 * @return 0 if sucess, error code < 0 on failure.
 */
int mmap_kernel(void);

/**
 * @brief Maps IO memory mapped address into the kernel.
 *
 * @param addr Addr of the memory mapped address space.
 * @param size The size of the mapped address.
 * @return 0 on SUCCESS, error code < 0 on FAILURE.
 */
int iommap_kernel(void *addr, size_t size);

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
