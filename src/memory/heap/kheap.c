#include "console/console.h"
#include "memory/heap/kheap.h"
#include "memory/heap/heap.h"
#include "memory/memory.h"
#include "config.h"
#include "status.h"

struct heap kernel_heap;
struct heap_table kernel_heap_table;

void kheap_init()
{
    int total_table_entries = HEAP_SIZE_BYTES / HEAP_BLOCK_SIZE;

    kernel_heap_table.entries = (HEAP_BLOCK_TABLE_ENTRY *)(HEAP_TABLE_ADDRESS);
    kernel_heap_table.total = total_table_entries;

    void *end = (void *)(HEAP_ADDRESS + HEAP_SIZE_BYTES);

    status_t res = heap_create(&kernel_heap,
                               (void *)HEAP_ADDRESS,
                               end,
                               &kernel_heap_table);

    if (status_is_error(res))
    {
        panic_status("Failed to create kernel heap", res);
    }
}

void *kmalloc(size_t size)
{
    void *ptr = NULL;

    status_t res = heap_malloc(&kernel_heap, size, &ptr);
    if (status_is_error(res))
    {
        return NULL;
    }

    return ptr;
}

void *kzalloc(size_t size)
{
    void *ptr = kmalloc(size);

    if (ptr)
    {
        memset(ptr, 0, size);
    }

    return ptr;
}

void kfree(void *ptr)
{
    if (!ptr)
    {
        return;
    }

    heap_free(&kernel_heap, ptr);
}
