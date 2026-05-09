#ifndef KHEAP_H
#define KHEAP_H

#include "status.h"

#include "stdint.h"
#include "stddef.h"

/**
 * Initialize the global kernel heap allocator.
 *
 * @return None.
 */
void kheap_init();

/**
 * Allocate heap memory from the global kernel heap.
 *
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory, or NULL on failure.
 */
void* kmalloc(size_t size);

/**
 * Free a pointer previously allocated from the kernel heap.
 *
 * @param ptr Allocation pointer to free.
 * @return None.
 */
void kfree(void* ptr);

/**
 * Allocate heap memory and zero-initialize the returned bytes.
 *
 * @param size Number of bytes to allocate.
 * @return Pointer to zeroed memory, or NULL on failure.
 */
void* kzalloc(size_t size);

#endif /* KHEAP_H */