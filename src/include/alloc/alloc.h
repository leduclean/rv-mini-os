/**
 * @file
 * @brief Heap allocator over a statically allocated heap.
 */

#pragma once

/**
 * @brief Allocate memory in the heap.
 *
 * @note The requested size is aligned on 8 bytes. A free block is reused
 * when possible, and split if the leftover is big enough.
 *
 * @param size Requested memory size, in bytes.
 * @return Pointer to the allocated memory, NULL if the heap is full.
 */
void *kalloc(unsigned size);

/**
 * @brief Free a memory address of the heap.
 *
 * @note The freed block is merged with its free neighbours. Freeing NULL or
 * an already freed address is a no-op.
 *
 * @param memory_addr Address returned by a previous kalloc() call.
 */
void kfree(void *memory_addr);
