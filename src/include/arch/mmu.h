#pragma once

/**
 * @brief Helper to update the tlb cache.
 */
static inline void mmu_flush_tlb(void)
{
	__asm__ volatile("sfence.vma zero, zero" ::: "memory");
}

/**
 * @brief Update all the processus mapping of va in tlb.
 *
 * @param va A pointer to the virtual adress.
 */
static inline void mmu_update_va(void *va)
{
	__asm__ volatile("sfence.vma %0, zero" : : "r"(va) : "memory");
}
