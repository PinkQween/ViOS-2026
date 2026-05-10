#ifndef MULTIHEAP_H
#define MULTIHEAP_H

#include "heap.h"

enum
{
    MULTIHEAP_FLAG_EXTERNALLY_OWNED = 0b00000001,
    MULTIHEAP_FLAG_DEFRAGMENT_WITH_PAGING = 0b00000010,
};

struct multiheap_single_heap
{
    struct heap* heap;
    uint8_t flags;
    struct multiheap_single_heap* next;
};

struct multiheap
{
    struct heap* starting_heap;
    struct multiheap_single_heap* first_multiheap;
    void* max_end_data_address;
    size_t total_heaps;
};

/**
 * Initialize a multiheap with a starting heap.
 * 
 * @param starting_heap Initial heap to use for the multiheap. This heap will be used for all allocations and deallocations, and will be the first heap attempted for allocations.
 * @return Pointer to the initialized multiheap, or NULL on failure.
 */
struct multiheap* multiheap_new(struct heap* starting_heap);

/**
 * Add a new memory region to the multiheap.
 * 
 * @param multiheap Multiheap to add the region to.
 * @param ptr Base pointer of the region to add.
 * @param end End pointer of the region to add.
 * @param flags Flags for the region being added.
 * @return STATUS_OK on success, negative status_t on failure.
 */
status_t multiheap_add(struct multiheap* multiheap, void* ptr, void* end, uint8_t flags);

/**
 * Add a new heap to the multiheap.
 * The multiheap will take ownership of the heap and will free it when the multiheap is freed, unless the MULTIHEAP_FLAG_EXTERNALLY_OWNED flag is set.
 * 
 * @param multiheap Multiheap to add the heap to.
 * @param heap Heap to add to the multiheap. This heap must be initialized and valid.
 * @param flags Flags for the heap being added. If MULTIHEAP_FLAG_EXTERNALLY_OWNED is set, the multiheap will not free this heap when the multiheap is freed.
 * @return STATUS_OK on success, negative status_t on failure.
 */
status_t multiheap_add_heap(struct multiheap* multiheap, struct heap* heap, uint8_t flags);

/**
 * Add an existing heap to the multiheap without taking ownership. This is a convenience wrapper around multiheap_add_heap that sets the MULTIHEAP_FLAG_EXTERNALLY_OWNED flag.  
 * The multiheap will use this heap for allocations, but will not free it when the multiheap is freed.
 * 
 * @param multiheap Multiheap to add the heap to.
 * @param heap Heap to add to the multiheap. This heap must be initialized and valid.
 * @param flags Flags for the heap being added. This is passed to multiheap_add_heap, but the MULTIHEAP_FLAG_EXTERNALLY_OWNED flag will be set regardless of the value passed in this parameter.
 * @return STATUS_OK on success, negative status_t on failure.
 */
status_t multiheap_add_exsisting_heap(struct multiheap* multiheap, struct heap* heap, uint8_t flags);

/**
 * Free a pointer allocated from the multiheap.
 * The multiheap will attempt to free the pointer from each of its heaps in order until it finds the heap that owns the pointer. If the pointer is not owned by any heap in the multiheap, this function does nothing.
 * 
 * @param multiheap Multiheap to free the pointer from.
 * @param ptr Pointer to free. This must be a pointer previously allocated from one of the heaps in the multiheap.
 */
void multiheap_free(struct multiheap* multiheap, void* ptr);

/**
 * Allocate memory from the multiheap.
 * The multiheap will attempt to allocate the requested size from each of its heaps in order until it finds a heap that can satisfy the allocation. If no heap can satisfy the allocation, this function returns NULL.
 * 
 * @param multiheap Multiheap to allocate from.
 * @param size Number of bytes to allocate.
 * @param out_ptr Output pointer for allocated memory. This is set to the allocated pointer on success, or left unchanged on failure.
 * @return STATUS_OK on success, negative status_t on failure (e.g. if no heap can satisfy the allocation).
 */
status_t multiheap_palloc(struct multiheap* multiheap, size_t size, void** out_ptr);

/**
 * Allocate memory from multiheap without defragmentation. This is a convenience wrapper around multiheap_palloc that sets the MULTIHEAP_FLAG_DEFRAGMENT_WITH_PAGEING flag for all heaps in the multiheap, which causes the multiheap to skip any heap that has this flag set when attempting to allocate memory. This allows the caller to allocate memory from the multiheap without triggering defragmentation of any heaps that have this flag set.
 * 
 * @param multiheap Multiheap to allocate from.
 * @param size Number of bytes to allocate.
 * @param out_ptr Output pointer for allocated memory. This is set to the allocated pointer on success, or left unchanged on failure.
 * @return STATUS_OK on success, negative status_t on failure (e.g. if no heap can satisfy the allocation).
 */
status_t multiheap_alloc(struct multiheap* multiheap, size_t size, void** out_ptr);

#endif /* MULTIHEAP_H */