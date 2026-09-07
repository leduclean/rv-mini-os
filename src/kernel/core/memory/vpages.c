#include "vpages.h"
#include "minilib/stddef.h"
#include "pages.h"

/* Sv39 indirection levels */
#define LEVELS 3

/* Sv39 Virtual Address bit mappings */
#define VA_VPN0_SHIFT 12
#define VA_VPN0_MASK (0x1ffUL << VA_VPN0_SHIFT)
#define VA_VPN1_SHIFT 21
#define VA_VPN1_MASK (0x1ffUL << VA_VPN1_SHIFT)
#define VA_VPN2_SHIFT 30
#define VA_VPN2_MASK (0x1ffUL << VA_VPN2_SHIFT)

/* PTE PPN first indirection shift */
#define PTE_PPN0_SHIFT 10

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
	unsigned long addr = (unsigned long)va;
	unsigned long mask;
	unsigned long shift;
	switch (lvl) {
	case 0:
		mask = VA_VPN0_MASK;
		shift = VA_VPN0_SHIFT;
		break;
	case 1:
		mask = VA_VPN1_MASK;
		shift = VA_VPN1_SHIFT;
		break;
	case 2:
		mask = VA_VPN2_MASK;
		shift = VA_VPN2_SHIFT;
		break;
	}
	return (addr & mask) >> shift;
}

int map_page(pte_t *root, const void *va, void *pa, unsigned long flags)
{
	pte_t *table = root;

	// Error tracking elements
	void *alloc_tables[LEVELS] = { 0 };
	int error_lvl = -1;

	for (int lvl = LEVELS - 1; lvl > 0; lvl--) {
		pte_t *pte = &table[_get_va_vpn(va, lvl)];

		if (!(*pte & PTE_V)) {
			void *next = page_alloc();

			if (!next) {
				error_lvl = lvl;
				goto err_release_prv;
			}

			alloc_tables[lvl] = next;
			*pte = (PAGE_NUMBER(next) << PTE_PPN0_SHIFT) | PTE_V;
		}
		// Go to the next level of indirection
		table = (pte_t *)((*pte >> PTE_PPN0_SHIFT) << PAGE_SHIFT);
	}

	pte_t *leaf_pte = &table[_get_va_vpn(va, 0)];

	unsigned long pte_flags = flags | PTE_V | PTE_A;

	if (flags & PTE_W)
		pte_flags |= PTE_D;

	*leaf_pte = (PAGE_NUMBER(pa) << PTE_PPN0_SHIFT) | pte_flags;

	return 0;

err_release_prv:
	for (int lvl = LEVELS - 1; lvl > error_lvl; lvl--) {
		if (alloc_tables[lvl])
			page_free(alloc_tables[lvl]);
	}

	return LEVELS - 1 - error_lvl;
}

int map_range(pte_t *root, void *va, void *pa, size_t size, unsigned long flags)
{
	const uint8_t *vaddr = (const uint8_t *)va;
	uint8_t *paddr = (uint8_t *)pa;

	for (size_t off = 0; off < size; off += PAGE_SIZE) {
		int res = map_page(root, vaddr + off, paddr + off, flags);

		if (res < 0) {
			return -1;
		}
	}

	return 0;
}
