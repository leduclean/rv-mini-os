#include <stddef.h>

#include <lib/clist.h>
#include <lib/stdio.h>

#include <asm/asm_defs.h>
#include <asm/cpu.h>

#include <kernel/ldsym.h>
#include <kernel/pages.h>

/**
 * @typedef page
 * @brief Physical page.
 *
 * @note The idiom is that the physical frame contain an initialized node
 * when free and is dirty when allocated.
 *
 */
typedef struct page {
	clist_node_t node; //< Clist node for free list.
} page_t;

static clist_node_t free_pages;
static inline void _push_free(void *page)
{
	page_t *p = page;
	clist_init_node(&p->node);
	// LIFO
	clist_push_front(&free_pages, &p->node);
}

// Keep in sync with MEMORY in kernel.ld (checked by page_init()).
#define RAM_SIZE (128UL * 1024 * 1024)

/**
 * @brief Ref counter of every ram page, indexed by page_idx().
 *
 * @note Sized on the whole ram (a compile-time constant): the entries of
 * the kernel image pages are never used.
 */
static uint8_t page_rc[RAM_SIZE >> PAGE_SHIFT] = { 0 };

static inline size_t page_idx(const void *page)
{
	return ((uintptr_t)page - (uintptr_t)_ram_start) >> PAGE_SHIFT;
}

uint8_t page_get_ref_count(const void *page)
{
	return page_rc[page_idx(page)];
}

void *page_get(void *page)
{
	irq_flags_t state = irq_save();
	page_rc[page_idx(page)]++;
	irq_restore(state);
	return (void *)page;
}

void page_init(void)
{
	if ((uintptr_t)(_ram_end - _ram_start) != RAM_SIZE) {
		panic("pages.c RAM_SIZE out of sync with kernel.ld");
	}

	clist_init_node(&free_pages);
	for (char *p = _page_start; p < _page_end; p += PAGE_SIZE) {
		_push_free(p);
	}
}

void *page_alloc(void)
{
	irq_flags_t state = irq_save();

	void *addr = clist_pop_front(&free_pages);
	if (!addr) {
		goto err_restore_irq;
	}

	memset(addr, 0, PAGE_SIZE);
	page_get(addr);

err_restore_irq:
	irq_restore(state);
	return addr;
}

void page_put(void *page)
{
	irq_flags_t state = irq_save();
	if (page_get_ref_count(page) == 0) {
		panic("page_put() on a page nobody holds: double free");
	}
	if (--page_rc[page_idx(page)] == 0) {
		_push_free(page);
	}
	irq_restore(state);
}
