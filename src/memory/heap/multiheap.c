#include "memory/heap/multiheap.h"
#include "memory/paging/paging.h"
#include "status.h"
#include "console/console.h"

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

static bool multiheap_heap_allows_paging(struct multiheap_single_heap* heap)
{
    return heap->flags & MULTIHEAP_FLAG_DEFRAGMENT_WITH_PAGING;
}

struct multiheap* multiheap_new(struct heap* metadata_heap)
{
    struct multiheap* multiheap = heap_zalloc(metadata_heap, sizeof(struct multiheap));

    if (!multiheap)
    {
        return NULL;
    }

    multiheap->starting_heap = metadata_heap;
    multiheap->first_multiheap = 0;
    multiheap->total_heaps = 0;

    return multiheap;
}

static struct multiheap_single_heap* multiheap_last_node(struct multiheap* multiheap)
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

static void* multiheap_get_max_memory_end_address(struct multiheap* multiheap)
{
    uintptr_t max_address = 0;

    struct multiheap_single_heap* current = multiheap->first_multiheap;

    while (current)
    {
        uintptr_t heap_end_address = (uintptr_t)current->heap->end_address;

        if (heap_end_address >= max_address)
        {
           max_address = heap_end_address;
        }

        current = current->next;
    }

    return (void*)max_address;
}

static struct multiheap_single_heap* multiheap_find_node_for_address(struct multiheap* multiheap, void* address)
{
    struct multiheap_single_heap* current = multiheap->first_multiheap;

    while (current)
    {
        if (heap_is_address_within_heap(current->heap, address))
        {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

status_t multiheap_add_heap(struct multiheap* multiheap, struct heap* heap, uint8_t flags)
{
    if (!multiheap || !heap)
    {
        return STATUS_ERR(EINVAL);
    }

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
        struct multiheap_single_heap* last = multiheap_last_node(multiheap);
        last->next = single_heap;
    }

    multiheap->total_heaps++;
    multiheap->max_end_data_address = multiheap_get_max_memory_end_address(multiheap);

    return STATUS_OK;
}

status_t multiheap_add_existing_heap(struct multiheap* multiheap, struct heap* heap, uint8_t flags)
{
    return multiheap_add_heap(multiheap, heap, flags|MULTIHEAP_FLAG_EXTERNALLY_OWNED);
}

status_t multiheap_add_exsisting_heap(struct multiheap* multiheap, struct heap* heap, uint8_t flags)
{
    return multiheap_add_existing_heap(multiheap, heap, flags);
}

status_t multiheap_add(struct multiheap* multiheap, void* start_address, void* end_address, uint8_t flags)
{
    if (!multiheap || !start_address || !end_address ||
        (uintptr_t)end_address <= (uintptr_t)start_address)
    {
        return STATUS_ERR(EINVAL);
    }

    struct heap* heap = heap_zalloc(multiheap->starting_heap, sizeof(struct heap));
    if (!heap)
    {
        return STATUS_ERR(ENOMEM);
    }

    struct heap_table* table = heap_zalloc(multiheap->starting_heap, sizeof(struct heap_table));
    if (!table)
    {
        heap_free(multiheap->starting_heap, heap);
        return STATUS_ERR(ENOMEM);
    }

    // Calculate table size and starting address
    size_t total_size = (size_t)((uintptr_t)end_address - (uintptr_t)start_address);
    size_t total_blocks = total_size / HEAP_BLOCK_SIZE;
    size_t table_size = total_blocks * sizeof(HEAP_BLOCK_TABLE_ENTRY);
    void* heap_start = (void*)((uintptr_t)start_address + table_size);

    if (!paging_is_aligned(heap_start))
    {
        heap_start = paging_align_address(heap_start);
    }

    table->entries = (HEAP_BLOCK_TABLE_ENTRY*)start_address;
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
    struct multiheap_single_heap* paging_heap = NULL;
    struct multiheap_single_heap* physical_heap = NULL;
    void* real_physical_address = NULL;

    multiheap_get_heap_and_paging_heap_for_address(multiheap, ptr, &physical_heap, &paging_heap, &real_physical_address);

    if (paging_heap)
    {
        size_t block_count = heap_allocation_block_count(paging_heap->heap, real_physical_address);
        size_t starting_block = heap_address_to_block(paging_heap->heap, real_physical_address);
        size_t ending_block = starting_block + block_count;

        for (size_t i = starting_block; i < ending_block; i++)
        {
            void* virtual_address_for_block = (void*)((uintptr_t)paging_heap->heap->start_address + (i * HEAP_BLOCK_SIZE));
            void* physical_address_for_block = paging_get_physical_address(paging_current_descriptor(), virtual_address_for_block);

            multiheap_free(multiheap, physical_address_for_block);
        }
    
        heap_free(physical_heap->heap, real_physical_address);
    }    
    else if (physical_heap)
    {
        heap_free(physical_heap->heap, real_physical_address);
    }
}

void multiheap_free_heap(struct multiheap* multiheap)
{
    if (!multiheap)
    {
        return;
    }

    struct multiheap_single_heap* current = multiheap->first_multiheap;

    while (current)
    {
        struct multiheap_single_heap* next = current->next;

        if (!(current->flags & MULTIHEAP_FLAG_EXTERNALLY_OWNED))
        {
            heap_free(multiheap->starting_heap, current->heap);
            heap_free(multiheap->starting_heap, current->heap->table->entries);
        }

        heap_free(multiheap->starting_heap, current);
        current = next;
    }

    heap_free(multiheap->starting_heap, multiheap);
}

static void* multiheap_alloc_first_pass(struct multiheap* multiheap, size_t size)
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

void* multiheap_alloc_paging(struct multiheap* multiheap, size_t size, struct multiheap_single_heap** out_paging_heap)
{
    void* allocation_ptr = NULL;
    size_t total_required_blocks = size / HEAP_BLOCK_SIZE;
    struct multiheap_single_heap* current = multiheap->first_multiheap;

    while (current)
    {
        if (multiheap_heap_allows_paging(current) && current->heap->free_blocks >= total_required_blocks)
        {
            void* ptr = 0;
            status_t res = heap_malloc(current->heap, total_required_blocks * HEAP_BLOCK_SIZE, &ptr);

            if (status_is_ok(res))
            {
                *out_paging_heap = current;
                return ptr;
            }
        }

        current = current->next;
    }

    return NULL;
}

void* multiheap_alloc_second_pass(struct multiheap* multiheap, size_t size)
{
    if (!multiheap_is_ready(multiheap))
    {
        return NULL;
    }

    // Try to allocate with paging/defragmentation
    struct multiheap_single_heap* chosen_heap = NULL;
    void* defragmented_virt_addr = multiheap_alloc_paging(multiheap, size, &chosen_heap);

    if (defragmented_virt_addr && chosen_heap)
    {
        struct paging_desc* paging_desc = paging_current_descriptor();
        if (!paging_desc)
        {
            return NULL;
        }

        size_t aligned_size = heap_align_value_to_upper(size);
        size_t total_blocks = aligned_size / HEAP_BLOCK_SIZE;
        void* current_virt_addr = defragmented_virt_addr;

        // Map physical blocks to virtual addresses
        for (size_t i = 0; i < total_blocks; i++)
        {
            void* block_phys_addr = heap_zalloc(chosen_heap->heap, HEAP_BLOCK_SIZE);
            if (!block_phys_addr)
            {
                return NULL;
            }

            paging_map(paging_desc, current_virt_addr, block_phys_addr, 
                      PAGING_IS_WRITEABLE | PAGING_IS_PRESENT);
            current_virt_addr = (void*)((uintptr_t)current_virt_addr + HEAP_BLOCK_SIZE);
        }

        return defragmented_virt_addr;
    }

    return NULL;
}

status_t multiheap_palloc(struct multiheap* multiheap, size_t size, void** out_ptr)
{
    if (!multiheap || !out_ptr || size == 0)
    {
        return STATUS_ERR(EINVAL);
    }

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
    if (!multiheap || !out_ptr || size == 0)
    {
        return STATUS_ERR(EINVAL);
    }

    void* allocation_ptr = multiheap_alloc_first_pass(multiheap, size);

    if (allocation_ptr)
    {
        *out_ptr = allocation_ptr;
        return STATUS_OK;
    }

    return STATUS_ERR(ENOMEM);
}

struct multiheap_single_heap* multiheap_get_heap_for_address(struct multiheap* multiheap, void* address)
{
    struct multiheap_single_heap* current = multiheap->first_multiheap;

    while (current)
    {
        if (heap_is_address_within_heap(current->heap, address))
        {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

bool multiheap_is_address_virtual(struct multiheap* multiheap, void* address)
{
    return (uintptr_t)address >= (uintptr_t)multiheap->max_end_data_address;
}

bool multiheap_is_ready(struct multiheap* multiheap)
{
    if (!multiheap)
    {
        return false;
    }
    return (multiheap->flags & MULTIHEAP_FLAG_IS_READY) != 0;
}

bool multiheap_can_add_heap(struct multiheap* multiheap)
{
    if (!multiheap)
    {
        return false;
    }
    return !multiheap_is_ready(multiheap);
}

struct multiheap_single_heap* multiheap_get_paging_heap_for_address(struct multiheap* multiheap, void* address)
{
    struct multiheap_single_heap* current = multiheap->first_multiheap;

    while (current)
    {
        if (multiheap_heap_allows_paging(current) && heap_is_address_within_heap(current->heap, address))
        {
            return current;
        }

        current = current->next;
    }

    return NULL;
}

void* multiheap_virtual_address_to_physical(struct multiheap* multiheap, void* address)
{
    return (void*)((uintptr_t)address - (uintptr_t)multiheap->max_end_data_address);
}

void multiheap_get_heap_and_paging_heap_for_address(struct multiheap* multiheap, void* ptr, struct multiheap_single_heap** out_heap, struct multiheap_single_heap** out_paging_heap, void** real_physical_address)
{
    void* real_address = ptr;

    if (multiheap_is_address_virtual(multiheap, ptr))
    {
        *out_paging_heap = multiheap_get_paging_heap_for_address(multiheap, ptr);
        real_address = multiheap_virtual_address_to_physical(multiheap, ptr);
    }

    *out_heap = multiheap_get_heap_for_address(multiheap, real_address);
    *real_physical_address = real_address;
}

size_t multiheap_allocation_block(struct multiheap* multiheap, void* ptr)
{
    struct multiheap_single_heap* paging_heap = NULL;
    struct multiheap_single_heap* heap = NULL;
    struct multiheap_single_heap* heap_to_check = NULL;
    void* real_physical_address = NULL;

    multiheap_get_heap_and_paging_heap_for_address(multiheap, ptr, &heap, &paging_heap, &real_physical_address);

    if (paging_heap)
    {
        heap_to_check = paging_heap;
    }
    
    if (!heap_to_check)
    {
        return 0;
    }

    return heap_allocation_block_count(heap_to_check->heap, real_physical_address);
}

size_t multiheap_allocation_block_count(struct multiheap* multiheap, void* ptr)
{
    return multiheap_allocation_block(multiheap, ptr) * HEAP_BLOCK_SIZE;
}

void multiheap_paging_heap_free_block(void* ptr)
{
    paging_map(paging_current_descriptor(), ptr, NULL, 0);
}


void* multiheap_realloc(struct multiheap* multiheap, void* old_ptr, size_t new_size)
{
    struct multiheap_single_heap* paging_heap = NULL;
    struct multiheap_single_heap* phys_heap = NULL;
    struct multiheap_single_heap* heap_to_use = NULL;
    void* real_phys_addr = NULL;
    multiheap_get_heap_and_paging_heap_for_address(multiheap, old_ptr, &phys_heap, &paging_heap, &real_phys_addr);

    if (paging_heap)
    {
        panic("Reallocation not yet supported for virtual addresses whose address differs from physical address\n");
    }

    heap_to_use = phys_heap;
    if (!heap_to_use)
    {
        // Heap is NULL create a new allocation
        multiheap_alloc(multiheap, new_size, &old_ptr);
        return old_ptr;
    }

    return heap_realloc(heap_to_use->heap, old_ptr, new_size);
}

status_t multiheap_ready(struct multiheap* multiheap)
{
    multiheap->flags |= MULTIHEAP_FLAG_IS_READY;

    struct paging_desc* paging_desc = paging_current_descriptor();

    if (!paging_desc)
    {
        panic("No paging descriptor found when marking multiheap as ready");
    }

    void* max_end_address = multiheap_get_max_memory_end_address(multiheap);

    multiheap->max_end_data_address = max_end_address;

    struct multiheap_single_heap* current = multiheap->first_multiheap;

    while (current)
    {
        if (multiheap_heap_allows_paging(current))
        {
            void* paging_heap_starting_address = max_end_address + (uintptr_t)current->heap->start_address;
            void* paging_heap_ending_address = max_end_address + (uintptr_t)current->heap->end_address;

            struct heap_table* paging_heap_table = heap_zalloc(multiheap->starting_heap, sizeof(struct heap_table));
            
            paging_heap_table->entries = heap_zalloc(multiheap->starting_heap, current->heap->table->total * sizeof(HEAP_BLOCK_TABLE_ENTRY));
            paging_heap_table->total = current->heap->table->total;

            struct heap* paging_heap = heap_zalloc(multiheap->starting_heap, sizeof(struct heap));

            heap_create(paging_heap, paging_heap_starting_address, paging_heap_ending_address, paging_heap_table);

            paging_map_to(paging_current_descriptor(), paging_heap_starting_address, paging_heap_starting_address, paging_heap_ending_address, 0);
            heap_callbacks_set(paging_heap, NULL, multiheap_paging_heap_free_block);

            current->paging_heap = paging_heap;
        }

        current = current->next;
    }

    return STATUS_OK;
}