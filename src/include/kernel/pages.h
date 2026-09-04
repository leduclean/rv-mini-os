/**
 * @file
 * @brief This file give primitive to allocate and handle physical pages.
 */

#pragma once
#include "asm_defs.h"

// Defined in the linker script (address only, never read the value)
extern char _heap_start[], _heap_end[];

#define PAGE_ALIGN_UP(addr)                          \
	((((unsigned long)(addr) + PAGE_SIZE - 1)) & \
	 ~(unsigned long)(PAGE_SIZE - 1))

// First usable page and how many of them (128M of ram minus the kernel image).
// Link-time values: usable at runtime, not in a constant expression.
#define PAGE_BASE ((uintptr_t)PAGE_ALIGN_UP(_heap_start))
#define PAGE_END ((uintptr_t)_heap_end)
#define PAGE_COUNT ((PAGE_END - PAGE_BASE) >> PAGE_SHIFT)
#define PAGE_NUMBER(addr) ((unsigned long)addr >> PAGE_SHIFT)
/**
 * @brief Init the paging allocator. 
 */
void pages_init();

/**
 * @brief Allocate the first free page.
 */
void *page_alloc();

/**
 * @brief Free an allocated page with \ref page_alloc().
 *
 * @param page A pointer to the page to free.
 */
void page_free(void *page);
