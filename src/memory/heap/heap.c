#include "memory/heap/heap.h"
#include "status.h"
#include "memory/memory.h"

#include "stdbool.h"

static status_t heap_validate_table(void* ptr, void* end, struct heap_table* table)
{
    size_t total_size   = (size_t)((uintptr_t)end - (uintptr_t)ptr);
    size_t total_blocks = total_size / HEAP_BLOCK_SIZE;

    if (total_size < HEAP_MINIMUM_SIZE_BYTES) {
        return STATUS_ERR(EINVAL);
    }

    if (table->total != total_blocks) {
        return STATUS_ERR(EINVAL);
    }

    return STATUS_OK;
}

static bool heap_validate_alignment(void* ptr)
{
    return ((uintptr_t)ptr % HEAP_BLOCK_SIZE) == 0;
}

status_t heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table)
{
    if (!heap || !ptr || !end || !table) {
        return STATUS_ERR(EINVAL);
    }

    if (!heap_validate_alignment(ptr) || !heap_validate_alignment(end)) {
        return STATUS_ERR(EINVAL);
    }

    memset(heap, 0, sizeof(*heap));
    heap->start_address = ptr;
    heap->end_address = end;
    heap->table = *table;

    status_t res = heap_validate_table(ptr, end, table);
    if (status_is_error(res)) {
        return res;
    }

    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

    return STATUS_OK;
}

uintptr_t heap_align_value_to_upper(uintptr_t value)
{
    if (value % HEAP_BLOCK_SIZE) {
        return value + (HEAP_BLOCK_SIZE - (value % HEAP_BLOCK_SIZE));
    }

    return value;
}

uintptr_t heap_align_value_to_lower(uintptr_t value)
{
    if (value % HEAP_BLOCK_SIZE) {
        return value - (value % HEAP_BLOCK_SIZE);
    }

    return value;
}

static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    return entry & 0x0F;
}

status_t heap_get_start_block(struct heap* heap, size_t blocks_needed)
{
    struct heap_table* table = &heap->table;

    size_t block_count = 0;
    int64_t start_block = -1;

    for (size_t i = 0; i < table->total; i++) {
        if (heap_get_entry_type(table->entries[i]) == HEAP_BLOCK_TABLE_ENTRY_FREE) {
            if (start_block == -1) {
                start_block = (int64_t)i;
            }

            block_count++;

            if (block_count == blocks_needed) {
                return (status_t)start_block;
            }
        } else {
            start_block = -1;
            block_count = 0;
        }
    }

    return STATUS_ERR(ENOMEM);
}

void* heap_block_to_address(struct heap* heap, size_t block)
{
    return (void*)((uintptr_t)heap->start_address + (block * HEAP_BLOCK_SIZE));
}

void heap_mark_blocks_taken(struct heap* heap, size_t start_block, size_t blocks_needed)
{
    size_t end_block = start_block + blocks_needed - 1;

    HEAP_BLOCK_TABLE_ENTRY entry =
        HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;

    if (blocks_needed > 1) {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    for (size_t i = start_block; i <= end_block; i++) {
        heap->table.entries[i] = entry;

        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;

        if (i != end_block) {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}

void heap_mark_blocks_free(struct heap* heap, size_t start_block)
{
    struct heap_table* table = &heap->table;

    for (size_t i = start_block; i < table->total; i++) {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;

        if (!(entry & HEAP_BLOCK_HAS_NEXT)) {
            break;
        }
    }
}

status_t heap_malloc_blocks(struct heap* heap, size_t blocks_needed)
{
    if (!heap) {
        return STATUS_ERR(EINVAL);
    }

    status_t start_block = heap_get_start_block(heap, blocks_needed);
    if (status_is_error(start_block)) {
        return start_block;
    }

    heap_mark_blocks_taken(heap, (size_t)start_block, blocks_needed);

    return start_block;
}

int heap_address_to_block(struct heap* heap, void* address)
{
    return (int)(((uintptr_t)address - (uintptr_t)heap->start_address) /
                 HEAP_BLOCK_SIZE);
}

status_t heap_malloc(struct heap* heap, size_t size, void** out_ptr)
{
    if (!heap || size == 0) {
        return STATUS_ERR(EINVAL);
    }

    size_t aligned_size   = heap_align_value_to_upper(size);
    size_t blocks_needed = aligned_size / HEAP_BLOCK_SIZE;

    status_t start_block = heap_malloc_blocks(heap, blocks_needed);
    if (status_is_error(start_block)) {
        return start_block;
    }

    void* addr = heap_block_to_address(heap, (size_t)start_block);

    if (out_ptr) {
        *out_ptr = addr;
    }

    return STATUS_OK;
}

void heap_free(struct heap* heap, void* ptr)
{
    if (!heap || !ptr) {
        return;
    }

    heap_mark_blocks_free(heap, heap_address_to_block(heap, ptr));
}

size_t heap_total_size(struct heap* heap)
{
    return heap->table.total * HEAP_BLOCK_SIZE;
}

size_t heap_total_used(struct heap* heap)
{
    size_t used_blocks = 0;

    for (size_t i = 0; i < heap->table.total; i++) {
        if (heap_get_entry_type(heap->table.entries[i]) == HEAP_BLOCK_TABLE_ENTRY_TAKEN) {
            used_blocks++;
        }
    }

    return used_blocks * HEAP_BLOCK_SIZE;
}

void* heap_zalloc(struct heap* heap, size_t size)
{
    void* ptr = 0;

    status_t res = heap_malloc(heap, size, &ptr);

    if (status_is_error(res)) {
        return NULL;
    }

    memset(ptr, 0, size);
    return ptr;
}

bool heap_is_address_in_heap(struct heap* heap, void* ptr)
{
    return (uintptr_t)ptr >= (uintptr_t)heap->start_address &&
           (uintptr_t)ptr < (uintptr_t)heap->end_address;
}