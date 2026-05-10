#ifndef KHEAP_H
#define KHEAP_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file kheap.h
 * @brief Kernel heap allocator interface.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

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
 * Allocate memory from the kernel heap.
 *
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory, or NULL on failure.
 */
void* kmalloc(size_t size);

/**
 * Allocate memory through the paging-aware allocation path.
 *
 * The current backend falls back to normal multiheap allocation first. Future
 * paging/defragmentation support belongs behind this interface.
 * 
 * @param size Number of bytes to allocate. 
 * @return Pointer to allocated memory, or NULL on failure.
 */
void* palloc(size_t size);

/**
 * Allocate zero-initialized memory from the kernel heap.
 * 
 * @param size Number of bytes to allocate.
 * @return Pointer to zero-initialized memory, or NULL on failure.
 */
void* kzalloc(size_t size);

/**
 * Kernel-internal implementation for the paging-aware allocation path.
 * 
 * @param size Number of bytes to allocate.
 * @return Pointer to allocated memory, or NULL on failure.
 */
void* kpalloc(size_t size);

/**
 * Reallocate memory from the kernel heap.
 *
 * @param ptr Pointer to the memory to reallocate.
 * @param new_size New size for the memory.
 * @return Pointer to the reallocated memory, or NULL on failure.
 */
void* krealloc(void* ptr, size_t new_size);

/**
 * Allocate zero-initialized memory through the paging-aware allocation path.
 * 
 * @param size Number of bytes to allocate.
 * @return Pointer to zero-initialized memory, or NULL on failure.
 */
void* kpzalloc(size_t size);

/**
 * Get the total managed heap size in bytes.
 *
 * @return Total managed heap size in bytes.
 */
size_t kheap_total_size();

/**
 * Get the total used heap size in bytes.
 *
 * @return Total used heap size in bytes.
 */
size_t kheap_total_used();

/**
 * Get the total free heap size in bytes.
 *
 * @return Total free heap size in bytes.
 */
size_t kheap_total_free();

/**
 * Free memory returned by kmalloc, kzalloc, palloc, or kpalloc.
 *
 * @param ptr Pointer to free.
 */
void kfree(void* ptr);

/**
 * Perform post-paging initialization for the kernel heap.
 * This is called after paging is enabled to set up any necessary state for the kernel heap to operate correctly in a paged environment, such as setting up paging heaps for each heap in the multiheap and mapping them to their corresponding virtual address ranges.
 * 
 * @return None.
 */
void kheap_post_paging();

#endif /* KHEAP_H */
