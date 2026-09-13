#include "pages.h"
#include "asm_defs.h"
#include "clist.h"
#include "cpu.h"
#include "minilib/stddef.h"
#include "minilib/stdio.h"

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

/**
 * @brief Ref Counter for shared page free
 *
 * @note The first entries are reserved because the first 
 * ram pages are allocated for the code itself. But only the
 * entire ram size is known at compile time.
 */
uint8_t page_rc[RAM_PAGE_COUNT] = { 0 };

uint8_t page_get_ref_count(const void *page)
{
	return page_rc[RAM_RELATIVE_PAGE_NUMBER(page)];
}

void page_get(const void *page)
{
	irq_flags_t state = irq_save();
	page_rc[RAM_RELATIVE_PAGE_NUMBER(page)]++;
	irq_restore(state);
}

void pages_init()
{
	clist_init_node(&free_pages);
	page_t *p;

	for (unsigned long i = 0; i < PAGE_COUNT; i++) {
		p = (page_t *)(i * PAGE_SIZE + PAGE_BASE);
		_push_free(p);
	}
}

void *page_alloc()
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
	if (--page_rc[RAM_RELATIVE_PAGE_NUMBER(page)] == 0) {
		_push_free(page);
	}
	irq_restore(state);
}
