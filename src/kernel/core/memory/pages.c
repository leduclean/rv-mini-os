#include "pages.h"
#include "asm_defs.h"
#include "clist.h"
#include "minilib/stddef.h"
#include "minilib/string.h"
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

void pages_init()
{
	clist_init_node(&free_pages);
	page_t *p;

	for (unsigned long i = 0; i < PAGE_COUNT; i++) {
		p = (page_t *)(i * PAGE_SIZE + PAGE_BASE);
		page_free(p);
	}
}

void *page_alloc()
{
	void *addr = clist_pop_front(&free_pages);
	memset(addr, 0, PAGE_SIZE);
	return addr;
}

void page_free(void *page)
{
	page_t *p = page;
	clist_init_node(&p->node);
	// LIFO
	clist_push_front(&free_pages, &p->node);
}
