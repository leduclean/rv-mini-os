#include "alloc.h"
#include "lib/clist.h"
#include "lib/container.h"
#include "lib/minilib/stddef.h"
#include "lib/minilib/stdint.h"
#include "minilib/string.h"

#define HEAPSIZE 64 * 1024
#define MIN_BLOCK_SIZE 32
#define ALIGN(x) (((x) + 7) & ~7)

/** @brief Statically allocated heap, no sbrk() syscall available yet. */
char heap[HEAPSIZE];
char *heap_end;

/** @brief Memory block allocated by kalloc(). */
typedef struct {
  unsigned size;            ///< Usable size of the block, in bytes.
  uint8_t free;             ///< 1 if the block is free, 0 if it is owned.
  clist_node_t memory_node; ///< Node in the heap ordered block list.
  clist_node_t free_node;   ///< Node in the free block list.
} block_t;

/** @brief Real block order in the heap. */
clist_node_t memory_list;

/** @brief Rapid free block list. */
clist_node_t free_list;

/** @brief Init the heap structure. */
void init_heap() {
  clist_init_node(&free_list);
  clist_init_node(&memory_list);
  memset(&heap, 0, sizeof(heap));
  heap_end = heap;
}

/**
 * @brief Init the list nodes of a block.
 *
 * @param block Block to init.
 */
static inline void init_nodes(block_t *block) {
  clist_init_node(&block->free_node);
  clist_init_node(&block->memory_node);
}
/**
 * @brief Boolean helper to find a free block with a sufficient size.
 *
 * @param node Free node of the current block.
 * @param arg Pointer to the needed size.
 * @return 1 if the block is big enough, 0 otherwise.
 */
static inline int _sufficient_size(clist_node_t *node, void *arg) {
  unsigned size = *(unsigned *)arg;
  block_t *block = container_of(node, block_t, free_node);
  return block->size >= size;
}

/**
 * @brief Create a block with a requested size.
 *
 * @param requested_size Size to allocate, in bytes.
 * @return New allocated block header, NULL if the heap is full.
 */
static block_t *new_bloc_with_size(unsigned requested_size) {
  unsigned heap_offset = requested_size + sizeof(block_t);
  if (heap_end + heap_offset >= heap + HEAPSIZE)
    return NULL;
  block_t *block = (block_t *)heap_end;
  init_nodes(block);
  block->size = requested_size;
  block->free = 0;
  clist_push_back(&block->memory_node, &memory_list);
  heap_end += heap_offset;
  return block;
}

/**
 * @brief Try to split a block when requesting it.
 *
 * @note The split only happens if the leftover reaches MIN_BLOCK_SIZE.
 *
 * @param block Block being allocated.
 * @param requested_size Requested size, in bytes.
 */
static void _check_split(block_t *block, unsigned requested_size) {
  unsigned unused = block->size - requested_size - sizeof(block_t);
  if (unused >= MIN_BLOCK_SIZE) {
    // Split
    block_t *split = (block_t *)((char *)(block + 1) + requested_size);
    init_nodes(split);
    split->size = unused;
    split->free = 1;
    clist_push_back(&split->free_node, &free_list);
    clist_insert_after(&block->memory_node, &split->memory_node);

    // Resize original one
    block->size = requested_size;
  }
}

void *kalloc(unsigned size) {
  size = ALIGN(size);
  clist_node_t *found_node = clist_find(&free_list, _sufficient_size, &size);
  if (!found_node) { // allocate a new block
    block_t *new_block = new_bloc_with_size(size);
    if (!new_block)
      return NULL;
    return (void *)(new_block + 1);
  }
  block_t *block = container_of(found_node, block_t, free_node);
  _check_split(block, size);
  block->free = 0;

  // Remove it from the free list
  clist_remove(&block->free_node);

  return (void *)(block + 1);
}

/**
 * @brief Merge two consecutive free blocks.
 *
 * @param block Merging block.
 * @param next Block merged into @p block.
 * @return The merged block if the merge was possible. Otherwise, return
 * unchanged @p block.
 */
static block_t *merge(block_t *block, block_t *next) {
  if (!block->free || !next->free)
    return block;
  // Remove the merged block (next) from the memory and free list.
  clist_remove(&next->memory_node);
  clist_remove(&next->free_node);

  // Give all the memory allocated by next to block.
  block->size += next->size + sizeof(block_t);
  return block;
}

/**
 * @brief Check if a block can be merged with the previous and/or the next.
 *
 * @param block Block to check on.
 * @return The merged block if any merge was possible. Otherwise, return
 * unchanged @p block.
 */
static block_t *check_merge(block_t *block) {
  clist_node_t *current_node = &block->memory_node;
  block_t *merged = NULL;
  if (current_node->next->in_list) {
    block_t *next = container_of(current_node->next, block_t, memory_node);
    if (next->free) {
      merged = merge(block, next);
    }
  }

  if (current_node->prev->in_list) {
    block_t *prev = container_of(current_node->prev, block_t, memory_node);
    if (prev->free) {
      merged = (merged) ? merge(prev, merged) : merge(prev, block);
    }
  }

  return (merged) ? merged : block;
}

void kfree(void *memory_addr) {
  if (!memory_addr)
    return;
  block_t *block = ((block_t *)(memory_addr)) - 1;
  if (block->free)
    return; // Already freed

  block->free = 1;
  block_t *merged = check_merge(block);
  if (!merged->free_node.in_list)
    clist_push_back(&free_list, &merged->free_node);
}
