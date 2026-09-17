/**
 * @file
 * @brief This file give the interface for the sv39 virtual paging.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

typedef unsigned long pte_t;
typedef uint32_t ppn_t;

/* Sv39 Memory mapping flags */
#define PTE_V (1 << 0)
#define PTE_R (1 << 1)
#define PTE_W (1 << 2)
#define PTE_X (1 << 3)
#define PTE_U (1 << 4)
#define PTE_G (1 << 5)
#define PTE_A (1 << 6)
#define PTE_D (1 << 7)

/* Software defined flags */
#define PTE_COW (1 << 8)
#define PTE_FLAGS_MASK ((1 << 10) - 1)

/**
 * @brief Memory map a virtual address to physical address of a page table.
 *
 * @note @root MUST be allocated via page_alloc(). The rest of the tree is entirely 
 * constructed from this function. 
 *
 * @param root The root table of the page table tree.
 * @param va The virtual address to map.
 * @param pa The physical page address.
 * @param flags Memory access flags.
 */
int vpage_map(pte_t *root, const void *va, void *pa, unsigned long flags);

static inline int vpage_map_user(pte_t *root, const void *va, void *pa,
				 unsigned long flags)
{
	return vpage_map(root, va, pa, PTE_U | flags);
}

/**
 * @brief Memory map a virtual address range to a physical address range of a page table.
 *
 * @note @root MUST be allocated via page_alloc(). The rest of the tree is entirely 
 * constructed from this function. 
 *
 * @param root The root table of the page table tree.
 * @param va The virtual address to map.
 * @param pa The physical address.
 * @param size The size of the memory range to map.
 * @param flags Memory access flags.
 */
int vpage_map_range(pte_t *root, void *va, void *pa, size_t size,
		    unsigned long flags);

static inline int vpage_map_user_range(pte_t *root, void *va, void *pa,
				       size_t size, unsigned long flags)
{
	return vpage_map_range(root, va, pa, size, PTE_U | flags);
}

/**
 * @brief Release the whole sv39 tree from the root itself 
 *
 * @param root The root table of the tree
 */
void vpage_tree_free(pte_t *root);

/**
 * @brief Set up lazy copy of a whole sv39 tree.
 *
 * This function use CoW (Copy on Write) policy to ensure we don't copy the entire
 * page.
 *
 * @param dst The destination root page directory.
 * @param src The source root page directory.
 * @return 0 on SUCCES else error code < 0.
 */
int vpage_tree_copy(pte_t *dst, pte_t *src);

/**
 * @brief Handle a CoW access to a write page.
 * 
 * @note This function has side effects on the tree itselfs,
 * copying a page to make it writable. But it does not affects 
 * the content at va.
 *
 * @param root A pointer to the root page directory.
 * @param va The virtual address of the CoW page.
 * @return 0 on SUCCESS else error code < 0.
 */
int vpage_handle_cow(pte_t *root, const void *va);

/**
 * @brief Copy @count byte from a virtual address to a destination.
 *
 * @param root The root page directory of emitted va.
 * @param dst The destination of the copy.
 * @param va The virtual address emitted by the process.
 * @param count The number of byte to copy.
 * @return 0 on SUCCESS else error code <0.
 */
int vpage_copyin(pte_t *root, void *dst, const void *va, size_t count);

/**
 * @brief Copy in a kernel destination buffer a user virtual address.
 *
 * @param root The root page directory of emitted va.
 * @param dst The kernel buffer destination of the copy.
 * @param va The virtual address emitted by a user process.
 * @param max The max number of char to copy.
 * @return 0 on SUCCESS else error code < 0.
 */
int vpage_copyinstr(pte_t *root, char *dst, const char *va, size_t max);

/**
 * @brief Copy @count byte from a physical address to a virtual address.
 *
 * @param root The root page directory of emitted va.
 * @param va The virtual address emitted by the process to copy on.
 * @param src The src buf to copy from.
 * @param count The number of byte to copy.
 * @return 0 on SUCCESS else error code <0.
 */
int vpage_copyout(pte_t *root, void *va, const void *src, size_t count);
