/**
 * @file
 * @brief This file give primitive to allocate and handle physical pages.
 */

#pragma once
#include "asm_defs.h"
#include "minilib/stdint.h"
#include "minilib/string.h"

// Defined in the linker script (address only, never read the value)
extern char _heap_start[], _heap_end[];

#define PAGE_ALIGN_UP(addr)                          \
	((((unsigned long)(addr) + PAGE_SIZE - 1)) & \
	 ~(unsigned long)(PAGE_SIZE - 1))

// Values known at compile time
#define RAM_BASE 0x80000000
#define RAM_SIZE (128UL * 1024 * 1024)
#define RAM_END (RAM_BASE + RAM_SIZE)
#define RAM_PAGE_COUNT ((RAM_END - PAGE_ALIGN_UP(RAM_BASE)) >> PAGE_SHIFT)

// First usable page and how many of them (128M of ram minus the kernel image).
// Link-time values: usable at runtime, not in a constant expression.
#define PAGE_BASE ((uintptr_t)PAGE_ALIGN_UP(_heap_start))
#define PAGE_END ((uintptr_t)_heap_end)
#define PAGE_COUNT ((PAGE_END - PAGE_BASE) >> PAGE_SHIFT)
#define PAGE_NUMBER(addr) ((unsigned long)addr >> PAGE_SHIFT)

#define HEAP_RELATIVE_PAGE_NUMBER(addr) \
	(PAGE_NUMBER(addr) - PAGE_NUMBER(PAGE_BASE))
#define RAM_RELATIVE_PAGE_NUMBER(addr) \
	(PAGE_NUMBER(addr) - PAGE_NUMBER(RAM_BASE))

/**
 * @brief Init the paging allocator. 
 */
void page_init();

/**
 * @brief Allocate the first free page.
 */
void *page_alloc();

/**
 * @brief Drop a reference on @page, releasing it once nobody holds it.
 *
 * @note Mirrors page_alloc() and page_get(): a page comes back to the
 * allocator only when its last holder puts it.
 *
 * @param page A pointer to the page.
 */
void page_put(void *page);

/**
 * @brief Take a reference on @page, keeping it alive until a matching put.
 *
 * @param page A pointer to the page.
 */
void page_get(const void *page);

/**
 * @brief Get the reference counter of a page.
 *
 * @param page A pointer to the page.
 * @return The refcounter.
 */
uint8_t page_get_ref_count(const void *page);

/**
 * @brief Copy the content of a page.
 *
 * @param dst A pointer to the page destination to copy on.
 * @param src A pointer to the page source to copy from.
 */
static inline void page_copy(void *dst, const void *src)
{
	memcpy(dst, src, PAGE_SIZE);
}
