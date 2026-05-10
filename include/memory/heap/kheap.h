#ifndef KHEAP_H
#define KHEAP_H

#include "status.h"

#include "stdint.h"
#include "stddef.h"

/**
 * @file kheap.h
 * @brief Kernel heap allocator interface.
 *
 * This header defines the interface for the kernel heap allocator, which
 * provides dynamic memory management for the kernel. The kernel heap is
 * a critical component that allows the kernel to allocate and free memory
 * at runtime, enabling features such as dynamic data structures, buffers,
 * and more flexible memory usage.
 *
 * The kernel heap is typically implemented as a simple allocator that manages
 * a contiguous region of memory reserved for the kernel. It may use a variety
 * of allocation strategies (e.g., first-fit, best-fit) and may include features
 * such as coalescing free blocks and splitting larger blocks to satisfy smaller
 * allocation requests.
 *
 * This interface provides functions for initializing the kernel heap, allocating
 * and freeing memory, and retrieving a pointer to the global heap structure.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

/**
 * Initialize the global kernel heap allocator.
 *
 * @return None.
 */
void kheap_init();

void* kmalloc(size_t size);

/**
 * Alllocate with defragmentation and paging support. This is a more expensive allocation function that may trigger defragmentation of the heap and/or paging in additional memory if necessary to satisfy the allocation request.
 * 
 * @param size Number of bytes to allocate. 
 * @param out_ptr Output pointer for allocated memory. This is set to the allocated pointer on success, or left unchanged on failure.
 * @return STATUS_OK on success, negative status_t on failure (e.g. if the allocation fails).
 */
void* palloc(size_t size);

/**
 * Allocate zero-initialized memory from the kernel heap. This is a convenience wrapper around kmalloc that also zeroes the allocated memory.
 * 
 * @param size Number of bytes to allocate.
 * @param out_ptr Output pointer for allocated and zero-initialized memory. This is set to the allocated pointer on success, or left unchanged on failure.
 * @return STATUS_OK on success
 */
void* kzalloc(size_t size);

/**
 * Allocate memory from the kernel heap with specific alignment requirements.
 * This is a more expensive allocation function that may trigger defragmentation of the heap and/or paging in additional memory if necessary to satisfy the allocation request with the specified alignment.
 * 
 * @param size Number of bytes to allocate.
 * @param alignment Alignment requirement for the allocated memory.
 * @param out_ptr Output pointer for allocated memory. This is set to the allocated pointer on success, or left unchanged on failure.
 * @return STATUS_OK on success, negative status_t on failure (e.g. if the allocation fails).
 */
void* kpalloc(size_t size);

void kfree(void* ptr);

#endif /* KHEAP_H */
