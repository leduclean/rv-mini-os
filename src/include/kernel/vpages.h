/**
 * @file
 * @brief This file give the interface for the sv39 virtual paging.
 */

#pragma once

#include "minilib/stdint.h"
#include "minilib/stddef.h"

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
int map_page(pte_t *root, const void *va, void *pa, unsigned long flags);

static inline int map_upage(pte_t *root, const void *va, void *pa,
			    unsigned long flags)
{
	return map_page(root, va, pa, PTE_U | flags);
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
int map_range(pte_t *root, void *va, void *pa, size_t size,
	      unsigned long flags);

static inline int map_urange(pte_t *root, void *va, void *pa, size_t size,
			     unsigned long flags)
{
	return map_range(root, va, pa, size, PTE_U | flags);
}

/**
 * @brief Release the whole sv39 tree from the root itself 
 *
 * @param root The root table of the tree
 */
void tree_free(pte_t *root);
