#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lib/string.h>

#include <asm/asm_defs.h>

#include <kernel/mmap.h>
#include <kernel/pages.h>
#include <kernel/vpages.h>

/* Sv39 indirection levels */
#define LEVELS 3
#define OFFSET_MASK 0xfffUL

/* Sv39 Virtual Address bit mappings */
#define VA_VPN_SHIFT 12
#define VA_VPN_BITS 9
#define VA_VPN_MASK 0x1ffUL

#define PTE_PPN_SHIFT 10
#define PTE_PPN_SIZE 0x1ffUL

#define PTE_SHIFT 3
#define PTE_SIZE (1 << PTE_SHIFT)
#define PTE_ENTRY_PER_TABLE (PAGE_SIZE >> PTE_SHIFT)

/**
 * @brief Helper to get a lvl of indirection of vpn given a va.
 *
 * @warning The @lvl must be between 0 and LEVELS -1 
 *
 * @param va Virtual adress.
 * @param lvl The lvl of indirection in the tree.
 * @return The corresponding vpn.
 */
static inline unsigned long _get_va_vpn(const void *va, uint8_t lvl)
{
	return ((unsigned long)va >> (PAGE_SHIFT + VA_VPN_BITS * lvl)) &
	       VA_VPN_MASK;
}

static inline pte_t *_get_pte(pte_t *table, const void *va, int lvl)
{
	return &table[_get_va_vpn(va, lvl)];
}

static inline pte_t *_get_next_lvl_pte(const pte_t pte)
{
	return (pte_t *)((pte >> PTE_PPN_SHIFT) << PAGE_SHIFT);
}

int vpage_map(pte_t *root, const void *va, void *pa, unsigned long flags)
{
	pte_t *table = root;

	// Error tracking elements
	void *alloc_tables[LEVELS] = { 0 };
	int error_lvl = -1;

	for (int lvl = LEVELS - 1; lvl > 0; lvl--) {
		pte_t *pte = _get_pte(table, va, lvl);

		if (!(*pte & PTE_V)) {
			void *next = page_alloc();

			if (!next) {
				error_lvl = lvl;
				goto err_release_prv;
			}

			alloc_tables[lvl] = next;
			*pte = (PAGE_NUMBER(next) << PTE_PPN_SHIFT) | PTE_V;
		}

		// Go to the next level of indirection
		table = _get_next_lvl_pte(*pte);
	}

	pte_t *leaf_pte = _get_pte(table, va, 0);
	unsigned long pte_flags = flags | PTE_V | PTE_A;

	if (flags & PTE_W)
		pte_flags |= PTE_D;

	*leaf_pte = (PAGE_NUMBER(pa) << PTE_PPN_SHIFT) | pte_flags;

	return 0;

err_release_prv:
	for (int lvl = LEVELS - 1; lvl > error_lvl; lvl--) {
		if (alloc_tables[lvl])
			page_put(alloc_tables[lvl]);
	}

	return -1;
}

static inline bool _is_a_next_lvl_ptr(pte_t pte)
{
	return (!(pte & PTE_X) && !(pte & PTE_W) && !(pte & PTE_R));
}

/**
 * @brief Recursive free walk to the sv39 entries.
 *
 * @param table The root table of the walk.
 * @param lvl The number of indirection lvl in the walk.
 */
static void walk_free(pte_t *table, int lvl)
{
	for (int i = 0; i < PTE_ENTRY_PER_TABLE; i++) {
		pte_t pte = table[i];

		if (pte & PTE_V) {
			if (lvl > 0 && _is_a_next_lvl_ptr(pte)) {
				// Go to the next lvl and free all entry too
				walk_free(_get_next_lvl_pte(pte), lvl - 1);
			} else {
				// Leaf pte
				// Free only valid and not shared leaf
				if (pte & (PTE_V) && !(pte & PTE_G) &&
				    !_is_a_next_lvl_ptr(pte)) {
					void *page = _get_next_lvl_pte(pte);
					page_put((void *)page);
				}
				// Reset the entry
				table[i] = 0;
			}
		}
	}
	// Free the table itself
	page_put(table);
}

void vpage_tree_free(pte_t *root)
{
	if (!root) {
		return;
	}
	walk_free(root, LEVELS - 1);
};

int vpage_map_range(pte_t *root, void *va, void *pa, size_t size,
		    unsigned long flags)
{
	const uint8_t *vaddr = (const uint8_t *)va;
	uint8_t *paddr = (uint8_t *)pa;

	for (size_t off = 0; off < size; off += PAGE_SIZE) {
		int res = vpage_map(root, vaddr + off, paddr + off, flags);

		if (res < 0) {
			return -1;
		}
	}

	return 0;
}

/**
 * @brief Descend the sv39 tree down to the leaf entry mapping @va.
 *
 * @param lvl Receives the leaf level.
 * @return NULL if @va is not mapped, else a pointer to its leaf PTE.
 */
static pte_t *walk_leaf(pte_t *root, const void *va)
{
	pte_t *table = root;

	for (int l = LEVELS - 1; l >= 0; l--) {
		pte_t *pte = _get_pte(table, va, l);

		if (!(*pte & PTE_V)) {
			return NULL;
		}
		if (!_is_a_next_lvl_ptr(*pte)) {
			return pte;
		}
		table = _get_next_lvl_pte(*pte);
	}
	return NULL;
}

static inline uint32_t get_flags(pte_t pte)
{
	return pte & PTE_FLAGS_MASK;
}

static inline void set_cow(pte_t *leaf)
{
	if (!(*leaf & PTE_W)) {
		return;
	}
	*leaf &= ~PTE_W;
	*leaf |= PTE_COW;

	void *page = (void *)_get_next_lvl_pte(*leaf);
	page_get(page);
	return;
}

static inline void unset_cow(pte_t *leaf)
{
	if (!(*leaf & PTE_COW)) {
		return;
	}
	*leaf &= ~PTE_COW;
	*leaf |= PTE_W;

	void *page = (void *)_get_next_lvl_pte(*leaf);
	page_put(page);

	return;
}

//TODO: This function could transform using the SUM bit
// in sstatus to fast copy instead of going to the tree.
static inline void *_get_pa_from_va(pte_t *root, void *va,
				    unsigned long needed_flags)
{
	pte_t *leaf = walk_leaf(root, va);
	if (!leaf) {
		return NULL;
	}

	if ((needed_flags & PTE_W) && (*leaf & PTE_COW) &&
	    vpage_handle_cow(root, va) != 0) {
		return NULL;
	}

	if ((*leaf & needed_flags) != needed_flags) {
		return NULL;
	}

	unsigned long *page = _get_next_lvl_pte(*leaf);
	return (char *)page + ((unsigned long)va & OFFSET_MASK);
}

int vpage_copyin(pte_t *root, void *dst, void *va, size_t count)
{
	void *pa = _get_pa_from_va(root, va, PTE_V | PTE_U | PTE_R);
	if (!pa) {
		return -1;
	}

	memcpy(dst, pa, count);

	return 0;
}

int vpage_copyinstr(pte_t *root, char *dst, const char *va, size_t max)
{
	const char *pa = _get_pa_from_va(root, (void *)va,
					 PTE_V | PTE_U | PTE_R);
	if (!pa) {
		return -1;
	}

	// The clamp is what keeps strnlen inside the mapped page.
	size_t n = PAGE_SIZE - ((unsigned long)va & OFFSET_MASK);
	if (n > max) {
		n = max;
	}

	size_t size = strnlen(pa, n);
	if (size == n) {
		// No terminator symbol
		return -1;
	}

	memcpy(dst, pa, size + 1);
	return 0;
}

int vpage_copyout(pte_t *root, void *va, const void *src, size_t count)
{
	void *pa = _get_pa_from_va(root, va, PTE_V | PTE_U | PTE_W);
	if (!pa) {
		return -1;
	}

	memcpy(pa, src, count);

	return 0;
}

int vpage_handle_cow(pte_t *root, void *va)
{
	int res;

	pte_t *leaf = walk_leaf(root, va);
	if (!leaf || !(*leaf & PTE_COW)) {
		res = -1;
		goto out;
	}
	void *page = (void *)(_get_next_lvl_pte(*leaf));
	if (page_get_ref_count(page) == 1) {
		// We are the last that has access to this page
		*leaf &= ~PTE_COW;
		*leaf |= PTE_W;
		return 0;
	}

	void *new_page = page_alloc();
	if (!new_page) {
		res = -1;
		goto out;
	}

	page_copy(new_page, page);
	unset_cow(leaf);

	res = vpage_map(root, (void *)((unsigned long)va & ~OFFSET_MASK),
			new_page, get_flags(*leaf));
	if (res < 0) {
		goto err_free_page;
	}

	mmap_update_tlb();
	return 0;

err_free_page:
	page_put(new_page);
out:
	return res;
}

static int _copy_level(pte_t *dst, pte_t *src, int lvl, unsigned long va)
{
	for (int i = 0; i < PTE_ENTRY_PER_TABLE; i++) {
		pte_t pte = src[i];
		if (!(pte & PTE_V))
			continue;

		unsigned long child_va = va |
					 ((unsigned long)i
					  << (PAGE_SHIFT + VA_VPN_BITS * lvl));

		int res;
		if (_is_a_next_lvl_ptr(pte)) {
			res = _copy_level(dst, _get_next_lvl_pte(pte), lvl - 1,
					  child_va);
			if (res < 0) {
				return res;
			}

		} else {
			// leaf level
			void *page = _get_next_lvl_pte(pte);

			// Set up CoW on every writable page
			// else than TRAPFRAME
			if (child_va != TRAPFRAME) {
				if (pte & PTE_W) {
					set_cow(&src[i]);
				}

				res = vpage_map(dst, (void *)child_va, page,
						get_flags(src[i]));
				if (res < 0) {
					return res;
				}
			}
		}
	}
	return 0;
}

int vpage_tree_copy(pte_t *dst, pte_t *src)
{
	if (!dst || !src) {
		return -1;
	}
	int res;
	res = _copy_level(dst, src, LEVELS - 1, 0);
	if (res < 0) {
		return res;
	}

	mmap_update_tlb();
	return 0;
}
