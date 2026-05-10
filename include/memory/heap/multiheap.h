#ifndef MULTIHEAP_H
#define MULTIHEAP_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file multiheap.h
 * @brief Multi-region heap allocator interface.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include "heap.h"

struct multiheap;
struct multiheap_single_heap;

enum
{
    MULTIHEAP_FLAG_EXTERNALLY_OWNED = 0b00000001,
    MULTIHEAP_FLAG_DEFRAGMENT_WITH_PAGING = 0b00000010,
};

/**
 * Linked-list node for one heap managed by a multiheap.
 */
struct multiheap_single_heap
{
    struct heap* heap;
    struct heap* paging_heap;
    int flags;
    struct multiheap_single_heap* next;
};

enum
{
    MULTIHEAP_FLAG_IS_READY = 0x01,
};

/**
 * Multiheap allocator state.
 *
 * A multiheap chains several fixed-block heaps and tries them in insertion
 * order. The starting heap is also used for allocator bookkeeping.
 */
struct multiheap
{
    // This heap is used to allocate space for the multiheap.
    struct heap* starting_heap;

    // The linked list for the first heap
    struct multiheap_single_heap* first_multiheap;

    void* max_end_data_address;
    int flags;
    size_t total_heaps;
};
/**
 * Create a multiheap descriptor using an existing heap for metadata.
 * 
 * @param starting_heap Heap used to allocate multiheap bookkeeping objects.
 * @return Pointer to the initialized multiheap, or NULL on failure.
 */
struct multiheap* multiheap_new(struct heap* starting_heap);

/**
 * Add a new memory region to the multiheap.
 * 
 * @param multiheap Multiheap to add the region to.
 * @param ptr Base pointer of the region to add. Heap metadata is stored at
 * the beginning of this range.
 * @param end End pointer of the region to add.
 * @param flags Flags for the region being added.
 * @return STATUS_OK on success, negative status_t on failure.
 */
status_t multiheap_add(struct multiheap* multiheap, void* ptr, void* end, uint8_t flags);

/**
 * Free a pointer allocated from the multiheap.
 * The multiheap will attempt to free the pointer from each of its heaps in order until it finds the heap that owns the pointer. If the pointer is not owned by any heap in the multi
 * heap, this function does nothing.
 * 
 * @param multiheap Multiheap to free the pointer from.
 * @param ptr Pointer to free. This must be a pointer previously allocated from one of the heaps in the multiheap.
 */
void multiheap_free(struct multiheap* multiheap, void* ptr);

/**
 * Add a new heap to the multiheap.
 * 
 * @param multiheap Multiheap to add the heap to.
 * @param heap Heap to add to the multiheap. This heap must be initialized and valid.
 * @param flags Flags for the heap being added.
 * @return STATUS_OK on success, negative status_t on failure.
 */
status_t multiheap_add_heap(struct multiheap* multiheap, struct heap* heap, uint8_t flags);

/**
 * Add an existing heap to the multiheap without taking ownership.
 * 
 * @param multiheap Multiheap to add the heap to.
 * @param heap Heap to add to the multiheap. This heap must be initialized and valid.
 * @param flags Additional flags for the heap being added.
 * @return STATUS_OK on success, negative status_t on failure.
 */
status_t multiheap_add_existing_heap(struct multiheap* multiheap, struct heap* heap, uint8_t flags);

/**
 * Compatibility alias for the old misspelled API name.
 */
status_t multiheap_add_exsisting_heap(struct multiheap* multiheap, struct heap* heap, uint8_t flags);

/**
 * Free a pointer allocated from the multiheap.
 * The multiheap will attempt to free the pointer from each of its heaps in order until it finds the heap that owns the pointer. If the pointer is not owned by any heap in the multiheap, this function does nothing.
 * 
 * @param multiheap Multiheap to free the pointer from.
 * @param ptr Pointer to free. This must be a pointer previously allocated from one of the heaps in the multiheap.
 */
void multiheap_free_heap(struct multiheap* multiheap);

/**
 * Check if the multiheap is ready for paging operations.
 * 
 * @param multiheap Multiheap to check.
 * @return true if ready, false otherwise.
 */
bool multiheap_is_ready(struct multiheap* multiheap);

/**
 * Mark the multiheap as ready for paging operations.
 * This sets up paging heaps for each heap in the multiheap that allows paging and maps them to their corresponding virtual address ranges.
 * 
 * @param multiheap Multiheap to mark as ready.
 * @return STATUS_OK on success, negative status_t on failure.
 */
status_t multiheap_ready(struct multiheap* multiheap);

/**
 * Check if heaps can still be added to the multiheap.
 * Heaps cannot be added after the multiheap is marked as ready.
 * 
 * @param multiheap Multiheap to check.
 * @return true if heaps can be added, false otherwise.
 */
bool multiheap_can_add_heap(struct multiheap* multiheap);

/**
 * Resolve a pointer to the owning heap and optional paging heap.
 */
void multiheap_get_heap_and_paging_heap_for_address(struct multiheap* multiheap, void* ptr, struct multiheap_single_heap** out_heap, struct multiheap_single_heap** out_paging_heap, void** real_physical_address);

/**
 * Allocate memory from the multiheap, allowing future expensive recovery work.
 *
 * The first pass tries each heap in insertion order. The second pass is
 * reserved for paging/defragmentation support and currently returns ENOMEM.
 * 
 * @param multiheap Multiheap to allocate from.
 * @param size Number of bytes to allocate.
 * @param out_ptr Output pointer for allocated memory.
 * @return STATUS_OK on success, negative status_t on failure.
 */
status_t multiheap_palloc(struct multiheap* multiheap, size_t size, void** out_ptr);

/**
 * Allocate memory from the first heap that can satisfy the request.
 * 
 * @param multiheap Multiheap to allocate from.
 * @param size Number of bytes to allocate.
 * @param out_ptr Output pointer for allocated memory.
 * @return STATUS_OK on success, negative status_t on failure.
 */
status_t multiheap_alloc(struct multiheap* multiheap, size_t size, void** out_ptr);

/**
 * Reallocate memory from the multiheap.
 * This is a placeholder for future paging/defragmentation support and currently only supports reallocating virtual addresses whose physical address is the same as the virtual address (i.e. addresses allocated from heaps that do not allow paging). Reallocation of other addresses will panic for now.
 * 
 * @param multiheap Multiheap to reallocate from.
 * @param old_ptr Pointer to previously allocated memory to reallocate. This must be a pointer previously allocated from the multiheap.
 * @param new_size New size in bytes for the allocation.
 * @return Pointer to reallocated memory on success, or NULL on failure.
 */
void* multiheap_realloc(struct multiheap* multiheap, void* old_ptr, size_t new_size);

#endif /* MULTIHEAP_H */