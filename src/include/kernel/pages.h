/**
 * @file
 * @brief This file give primitive to allocate and handle physical pages.
 */

#pragma once
#include <stdint.h>

#include <lib/bit.h>
#include <lib/string.h>

#include <asm/asm_defs.h>

/**
 * @brief Gets the number associated with an address of a page.
 *
 * @param addr page addr.
 * @return Page number of this addr.
 */
static inline unsigned long page_get_num(const void *addr)
{
	return (uintptr_t)addr >> PAGE_SHIFT;
}

/**
 * @brief Init the paging allocator. 
 */
void page_init(void);

/**
 * @brief Allocate the first free page.
 */
void *page_alloc(void);

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
 * @return Another pointer to the page.
 */
void *page_get(void *page);

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
