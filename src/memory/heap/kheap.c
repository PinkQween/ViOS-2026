#include "console/console.h"
#include "memory/heap/kheap.h"
#include "memory/heap/heap.h"
#include "memory/memory.h"
#include "config.h"
#include "status.h"
#include "memory/heap/multiheap.h"
#include "memory/paging/paging.h"

struct heap kernel_minimal_heap;
struct heap_table kernel_minimal_heap_table;

struct multiheap* kernel_multiheap;

struct e820_entry* kheap_get_allowable_memory_region_for_minimal_heap()
{
    struct e820_entry* entry = 0;
    size_t total_entries = e820_total_entries();

    for (size_t i = 0; i < total_entries; i++)
    {
        struct e820_entry* current_entry = e820_get_entry(i);
        uintptr_t region_start = (uintptr_t)current_entry->base_addr;
        uintptr_t region_end = region_start + (uintptr_t)current_entry->length;

        if (region_end > KERNEL_BOOT_IDENTITY_MAP_LIMIT)
        {
            region_end = KERNEL_BOOT_IDENTITY_MAP_LIMIT;
        }

        if (current_entry->type == 1 &&
            region_end > region_start &&
            region_end - region_start >= HEAP_MINIMUM_SIZE_BYTES)
        {
            entry = current_entry;
            break;
        }
    }

    return entry;
}

static bool kheap_range_is_usable(uintptr_t start, size_t size)
{
    uint16_t total_entries = *((uint16_t*)MEMORY_MAP_TOTAL_ENTRIES_LOCATION);
    struct e820_entry* entries = (struct e820_entry*)MEMORY_MAP_LOCATION;
    uintptr_t end = start + size;

    if (end < start)
    {
        return false;
    }

    for (uint16_t i = 0; i < total_entries; i++)
    {
        if (entries[i].type != 1)
        {
            continue;
        }

        uintptr_t region_start = (uintptr_t)entries[i].base_addr;
        uintptr_t region_end = region_start + (uintptr_t)entries[i].length;

        if (region_end < region_start)
        {
            continue;
        }

        if (start >= region_start && end <= region_end)
        {
            return true;
        }
    }

    return false;
}

void kheap_init()
{
    struct e820_entry* entry = kheap_get_allowable_memory_region_for_minimal_heap();

    if (!entry)
    {
        panic("Installed random access memory map does not contain a suitable region for the kernel heap");
    }

    void* address = (void*)entry->base_addr;
    uintptr_t entry_end = (uintptr_t)entry->base_addr + (uintptr_t)entry->length;
    if (entry_end > KERNEL_BOOT_IDENTITY_MAP_LIMIT)
    {
        entry_end = KERNEL_BOOT_IDENTITY_MAP_LIMIT;
    }

    void* end_address = (void*)entry_end;
    void* heap_table_address = address;

    if ((uintptr_t)heap_table_address < (uintptr_t)MINIMAL_HEAP_TABLE_ADDRESS)
    {
        heap_table_address = (void*)MINIMAL_HEAP_TABLE_ADDRESS;
    }

    size_t total_heap_size = (uintptr_t)end_address - (uintptr_t)heap_table_address;
    size_t total_heap_blocks = total_heap_size / HEAP_BLOCK_SIZE;
    size_t total_heap_entry_table_size = total_heap_blocks * sizeof(HEAP_BLOCK_TABLE_ENTRY);
    size_t heap_data_size = total_heap_size - total_heap_entry_table_size;
    size_t total_heap_data_blocks = heap_data_size / HEAP_BLOCK_SIZE;
    
    total_heap_entry_table_size = total_heap_data_blocks * sizeof(HEAP_BLOCK_TABLE_ENTRY);

    if ((uintptr_t)heap_table_address + total_heap_entry_table_size >= (uintptr_t)end_address)
    {
        panic("Calculated heap entry table size exceeds available memory region");
    }

    void* heap_address = (void*)((uintptr_t)heap_table_address + total_heap_entry_table_size);
    void* heap_end_address = end_address;

    if (total_heap_data_blocks == 0)
    {
        panic("Calculated heap data size is too small to be usable");
    }

    if (!paging_is_aligned(heap_address))
    {
        heap_address = paging_align_address(heap_address);
    }

    if (!paging_is_aligned(heap_end_address))
    {
        heap_end_address = paging_align_to_lower_page(heap_end_address);
    }

    size_t size = (uintptr_t)heap_end_address - (uintptr_t)heap_address;
    size_t total_table_entries = size / HEAP_BLOCK_SIZE;

    kernel_minimal_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY*)heap_table_address;
    kernel_minimal_heap_table.total = total_table_entries;

    if (status_is_error(heap_create(&kernel_minimal_heap, heap_address, heap_end_address, &kernel_minimal_heap_table)))
    {
        panic("Failed to create kernel minimal heap");
    }

    kernel_multiheap = multiheap_new(&kernel_minimal_heap);
    if (!kernel_multiheap)
    {
        panic("Failed to create kernel multiheap");
    }

    multiheap_add_exsisting_heap(kernel_multiheap, &kernel_minimal_heap, MULTIHEAP_FLAG_EXTERNALLY_OWNED);

    struct e820_entry* used_entry = entry;

    size_t total_entries = e820_total_entries();

    for (size_t i = 0; i < total_entries; i++)
    {
        struct e820_entry* current_entry = e820_get_entry(i);

        if (current_entry->type == 1 &&
            current_entry != used_entry)
        {
            void* base_address = (void*)current_entry->base_addr;
            void* end_address = (void*)(current_entry->base_addr + current_entry->length);

            if ((uintptr_t)base_address >= KERNEL_BOOT_IDENTITY_MAP_LIMIT)
            {
                continue;
            }

            if ((uintptr_t)end_address > KERNEL_BOOT_IDENTITY_MAP_LIMIT)
            {
                end_address = (void*)KERNEL_BOOT_IDENTITY_MAP_LIMIT;
            }

            if (!paging_is_aligned(base_address))
            {
                base_address = paging_align_address(base_address);
            }

            if (!paging_is_aligned(end_address))
            {
                end_address = paging_align_to_lower_page(end_address);
            }

            if (base_address < (void*)MINIMAL_HEAP_ADDRESS)
            {
                base_address = (void*)MINIMAL_HEAP_ADDRESS;
            }

            if (end_address <= base_address)
            {
                continue;
            }

            multiheap_add(kernel_multiheap, base_address, end_address, MULTIHEAP_FLAG_DEFRAGMENT_WITH_PAGING);
        }
    }
}

void* kmalloc(size_t size)
{
    void* ptr = NULL;
    status_t res = multiheap_alloc(kernel_multiheap, size, &ptr);

    if (status_is_error(res))
    {
        return NULL;
    }

    return ptr;
}

void* kzalloc(size_t size)
{
    void* ptr = kmalloc(size);
    if (!ptr)
    {
        return NULL;
    }

    memset(ptr, 0, size);
    return ptr;
}

void* kpalloc(size_t size)
{
    void* ptr = NULL;
    status_t res = multiheap_palloc(kernel_multiheap, size, &ptr);

    if (status_is_error(res))
    {
        return NULL;
    }

    return ptr;
}

void* palloc(size_t size)
{
    return kpalloc(size);
}

void kfree(void* ptr)
{
    multiheap_free(kernel_multiheap, ptr);
}
