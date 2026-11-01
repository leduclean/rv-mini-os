/**
 * @file
 * @brief This file contain all the needed asm macro definition.
 */

#pragma once

#define MAX_VA (1L << 38)

// Each page as a size of 4096 bytes
#define PAGE_SHIFT 12
#define PAGE_SIZE (1 << PAGE_SHIFT)

#define TRAMPOLINE (MAX_VA - PAGE_SIZE)
#define TRAPFRAME (MAX_VA - 2 * PAGE_SIZE)

//TODO: Move the asm structure fields offset here instead
