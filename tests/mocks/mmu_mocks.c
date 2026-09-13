#include "mmu_mocks.h"
#include "vpages.h"
#include "kernel_mocks.h"

static inline unsigned long _get_pte_ppn(pte_t pte)
{
	return (pte & (PTE_PPN0_MASK | PTE_PPN1_MASK | PTE_PPN2_MASK)) >>
	       PTE_PPN0_SHIFT;
}

static inline uint8_t _get_va_vpn(const void *va, uint8_t lvl)
{
	unsigned long addr = (unsigned long)va;
	unsigned long mask;
	unsigned long shift;
	switch (lvl) {
	case 0:
		mask = VA_VPN0_MASK;
		shift = VA_VPN2_SHIFT;
		break;
	case 1:
		mask = VA_VPN1_MASK;
		shift = VA_VPN2_SHIFT;
		break;
	case 2:
		mask = VA_VPN2_MASK;
		shift = VA_VPN2_SHIFT;
		break;
	}
	return (addr & mask) >> shift;
}

static inline pte_t *_get_pte(pte_t *ptable, const void *va, uint8_t lvl)
{
	return (pte_t *)(ptable + _get_va_vpn(va, lvl) * PTESIZE);
}

static inline _Bool _valid_pte(pte_t pte)
{
	/* Sv39: V must be set, W without R is reserved, reserved bits must be 0 */
	if (!(pte & PTE_V) || ((pte & PTE_W) && !(pte & PTE_R)) ||
	    (pte & PTE_RESERVERD_MASK)) {
		return 0;
	}
	return 1;
}

static inline ppn_t _get_lvl_ppn(pte_t pte, uint8_t lvl)
{
	unsigned long mask;
	unsigned long shift;
	switch (lvl) {
	case 0:
		mask = PTE_PPN0_MASK;
		shift = PTE_PPN0_SHIFT;
		break;
	case 1:
		mask = PTE_PPN1_MASK;
		shift = PTE_PPN1_SHIFT;
		break;
	case 2:
		mask = PTE_PPN2_MASK;
		shift = PTE_PPN2_SHIFT;
		break;
	}
	return (pte & mask) >> shift;
}

static inline _Bool _valid_ppn(ppn_t ppn, uint8_t lvl)
{
	if ((lvl > 0) && (ppn & ((1 << lvl) - 1))) {
		return 0;
	}

	return 1;
}

static inline _Bool _user_mode_page(pte_t pte)
{
	return pte & PTE_U;
}

static inline _Bool _is_a_next_lvl_ptr(pte_t pte)
{
	return (!(pte & PTE_X) && !(pte & PTE_W) && !(pte & PTE_R));
}

static inline unsigned long _get_lvl_mask(uint8_t lvl)
{
	unsigned long mask = 0;

	// No break for mask incremental construction.
	switch (lvl) {
	case 0:
		mask |= PTE_PPN0_MASK;
	case 1:
		mask |= PTE_PPN1_MASK;
	case 2:
		mask |= PTE_PPN2_MASK;
	}
	return mask;
}

enum FAULTS {
	UNVALID_PTE = -3,
	UNVALID_PPN = -2,
	UNVALID_U_ACCESS = -1,
	VALID,

};

// FIXME: This could be a test function (this is implemented by the hardware itself)
int walk(pte_t *root, const void *va, void **pa)
{
	pte_t *ptable_frame = root;

	for (uint8_t lvl = LEVELS - 1; lvl >= 0; lvl--) {
		pte_t *pte_addr = _get_pte(ptable_frame, va, lvl);
		pte_t pte = *pte_addr;

		if (!_valid_pte(pte)) {
			return UNVALID_PTE;
		}

		if (_is_a_next_lvl_ptr(pte)) {
			ptable_frame = (pte_t *)(_get_pte_ppn(pte));
			continue;
		}

		ppn_t ppn = _get_lvl_ppn(pte, lvl);

		if (!_valid_ppn(ppn, lvl)) {
			return UNVALID_PPN;
		}

		// Supervisor should not access to user mode pages
		if (_user_mode_page(pte)) {
			return UNVALID_U_ACCESS;
		}

		if (!(pte & PTE_A)) {
			irq_flags_t flags = irq_save();
			pte_t check_pte = *_get_pte(ptable_frame, va, lvl);

			// Check if the value has changed during the previous operations.
			if (pte == check_pte) {
				// Update to set the A bit.
				*pte_addr = pte | PTE_A;
				irq_restore(flags);
			} else {
				// Retry without going to another indirection lvl.
				lvl++;
				continue;
			}
		};

		// The translation is successfull
		unsigned long mask = _get_lvl_mask(lvl);
		*pa = (void *)((ppn & mask) | ((unsigned long)va & ~mask));
		return VALID;
	}
}
