#include "memory/heap/multiheap.h"
#include "memory/paging/paging.h"
#include "status.h"

struct multiheap* multiheap_new(struct heap* starting_heap)
{
    struct multiheap* multiheap = heap_zalloc(starting_heap, sizeof(struct multiheap));

    if (!multiheap)
    {
        return NULL;
    }

    multiheap->starting_heap = starting_heap;
    multiheap->first_multiheap = 0;
    multiheap->total_heaps = 0;

    return multiheap;
}

struct multiheap_single_heap* multiheap_get_last_heap(struct multiheap* multiheap)
{
    struct multiheap_single_heap* current = multiheap->first_multiheap;

    while (current)
    {
        if (!current->next)
        {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

static bool multiheap_heap_allows_paging(struct multiheap_single_heap* single_heap)
{
    return single_heap->flags & MULTIHEAP_FLAG_DEFRAGMENT_WITH_PAGING;
}

void* multiheap_get_max_memory_end_address(struct multiheap* multiheap)
{
    void* max_address = 0;

    struct multiheap_single_heap* current = multiheap->first_multiheap;

    while (current)
    {
        if (current->heap->end_address >= max_address)
        {
           max_address = current->heap->end_address;
        }

        current = current->next;
    }

    return max_address;
}

struct multiheap_single_heap* multiheap_get_heap_for_address(struct multiheap* multiheap, void* address)
{
    struct multiheap_single_heap* current = multiheap->first_multiheap;

    while (current)
    {
        if (heap_is_address_in_heap(current->heap, address))
        {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

status_t multiheap_add_heap(struct multiheap* multiheap, struct heap* heap, uint8_t flags)
{
    struct multiheap_single_heap* single_heap = heap_zalloc(multiheap->starting_heap, sizeof(struct multiheap_single_heap));

    if (!single_heap)
    {
        return STATUS_ERR(ENOMEM);
    }

    single_heap->heap = heap;
    single_heap->flags = flags;
    single_heap->next = 0;

    if (!multiheap->first_multiheap)
    {
        multiheap->first_multiheap = single_heap;
    }
    else
    {
        struct multiheap_single_heap* last = multiheap_get_last_heap(multiheap);
        last->next = single_heap;
    }

    multiheap->total_heaps++;

    return STATUS_OK;
}

status_t multiheap_add_exsisting_heap(struct multiheap* multiheap, struct heap* heap, uint8_t flags)
{
    return multiheap_add_heap(multiheap, heap, flags|MULTIHEAP_FLAG_EXTERNALLY_OWNED);
}

status_t multiheap_add(struct multiheap* multiheap, void* start_address, void* end_address, uint8_t flags)
{
    struct heap* heap = heap_zalloc(multiheap->starting_heap, sizeof(struct heap));

    if (!heap)
    {
        return STATUS_ERR(ENOMEM);
    }

    struct heap_table* table = heap_zalloc(multiheap->starting_heap, sizeof(struct heap_table));

    if (!table)
    {
        return STATUS_ERR(ENOMEM);
    }

    size_t total_size = (size_t)((uintptr_t)end_address - (uintptr_t)start_address);
    size_t total_blocks = total_size / HEAP_BLOCK_SIZE;
    size_t table_size = total_blocks * sizeof(HEAP_BLOCK_TABLE_ENTRY);
    void* heap_start = (void*)((uintptr_t)start_address + table_size);

    if (!paging_is_aligned(heap_start))
    {
        heap_start = paging_align_address(heap_start);
    }

    table->entries = start_address;
    table->total = ((uintptr_t)end_address - (uintptr_t)heap_start) / HEAP_BLOCK_SIZE;

    status_t res = heap_create(heap, heap_start, end_address, table);

    if (status_is_error(res))
    {
        heap_free(multiheap->starting_heap, heap);
        heap_free(multiheap->starting_heap, table);
        return res;
    }

    return multiheap_add_heap(multiheap, heap, flags);
}

void multiheap_free(struct multiheap* multiheap, void* ptr)
{
    struct multiheap_single_heap* current = multiheap->first_multiheap;

    while (current)
    {
        struct heap* heap = current->heap;

        if ((uintptr_t)ptr >= (uintptr_t)heap->start_address &&
            (uintptr_t)ptr < (uintptr_t)heap->end_address)
        {
            heap_free(heap, ptr);
            return;
        }

        current = current->next;
    }
}

void* multiheap_alloc_first_pass(struct multiheap* multiheap, size_t size)
{
    struct multiheap_single_heap* current = multiheap->first_multiheap;

    while (current)
    {
        void* ptr = 0;
        status_t res = heap_malloc(current->heap, size, &ptr);

        if (status_is_ok(res))
        {
            return ptr;
        }

        current = current->next;
    }

    return NULL;
}

void* multiheap_alloc_second_pass(struct multiheap* multiheap, size_t size)
{
    // TODO: implement defragmentation and paging in multiheap
    (void)multiheap;
    (void)size;
    return NULL;
}

status_t multiheap_palloc(struct multiheap* multiheap, size_t size, void** out_ptr)
{
    void* allocation_ptr = multiheap_alloc_first_pass(multiheap, size);

    if (allocation_ptr)
    {
        *out_ptr = allocation_ptr;
        return STATUS_OK;
    }

    allocation_ptr = multiheap_alloc_second_pass(multiheap, size);
    
    if (allocation_ptr)
    {
        *out_ptr = allocation_ptr;
        return STATUS_OK;
    }

    return STATUS_ERR(ENOMEM);
}

status_t multiheap_alloc(struct multiheap* multiheap, size_t size, void** out_ptr)
{
    void* allocation_ptr = multiheap_alloc_first_pass(multiheap, size);

    if (allocation_ptr)
    {
        *out_ptr = allocation_ptr;
        return STATUS_OK;
    }

    return STATUS_ERR(ENOMEM);
}

bool multiheap_is_address_virtual(struct multiheap* multiheap, void* address)
{
    return address >= multiheap->max_end_data_address;
}

void* multiheap_virtual_address_to_physical(struct multiheap* multiheap, void* address)
{
    return (void*)((uintptr_t)address - (uintptr_t)multiheap->max_end_data_address);
}
