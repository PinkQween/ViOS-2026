#include "memory/heap/heap.h"
#include "status.h"
#include "memory/memory.h"

#include <stdbool.h>

static status_t heap_validate_table(void* ptr, void* end, struct heap_table* table)
{
    size_t total_size   = (size_t)((uintptr_t)end - (uintptr_t)ptr);
    size_t total_blocks = total_size / HEAP_BLOCK_SIZE;

    if (total_size < HEAP_SIZE_BYTES) {
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
    heap->table         = *table;

    status_t res = heap_validate_table(ptr, end, table);
    if (status_is_error(res)) {
        return res;
    }

    size_t table_size = sizeof(HEAP_BLOCK_TABLE_ENTRY) * table->total;
    memset(table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size);

    return STATUS_OK;
}

static uint32_t heap_align_value_to_upper(uint32_t value)
{
    uint32_t remainder = value % HEAP_BLOCK_SIZE;
    if (remainder == 0) {
        return value;
    }

    return value + (HEAP_BLOCK_SIZE - remainder);
}

static int heap_get_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    return entry & 0x0F;
}

status_t heap_get_start_block(struct heap* heap, uint32_t blocks_needed)
{
    struct heap_table* table = &heap->table;

    uint32_t block_count = 0;
    int start_block      = -1;

    for (uint32_t i = 0; i < table->total; i++) {
        if (heap_get_entry_type(table->entries[i]) == HEAP_BLOCK_TABLE_ENTRY_FREE) {
            if (block_count == 0) {
                start_block = (int)i;
            }

            block_count++;

            if (block_count == blocks_needed) {
                return (status_t)start_block;
            }
        } else {
            block_count = 0;
            start_block = -1;
        }
    }

    return STATUS_ERR(ENOMEM);
}

void* heap_block_to_address(struct heap* heap, uint32_t block)
{
    return (void*)((uintptr_t)heap->start_address + (block * HEAP_BLOCK_SIZE));
}

void heap_mark_blocks_taken(struct heap* heap, uint32_t start_block, uint32_t blocks_needed)
{
    uint32_t end_block = start_block + blocks_needed - 1;

    HEAP_BLOCK_TABLE_ENTRY entry =
        HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;

    if (blocks_needed > 1) {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    for (uint32_t i = start_block; i <= end_block; i++) {
        heap->table.entries[i] = entry;

        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;

        if (i != end_block) {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}

void heap_mark_blocks_free(struct heap* heap, uint32_t start_block)
{
    struct heap_table* table = &heap->table;

    for (uint32_t i = start_block; i < table->total; i++) {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;

        if (!(entry & HEAP_BLOCK_HAS_NEXT)) {
            break;
        }
    }
}

status_t heap_malloc_blocks(struct heap* heap, uint32_t blocks_needed)
{
    if (!heap) {
        return STATUS_ERR(EINVAL);
    }

    status_t start_block = heap_get_start_block(heap, blocks_needed);
    if (status_is_error(start_block)) {
        return start_block;
    }

    heap_mark_blocks_taken(heap, (uint32_t)start_block, blocks_needed);

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

    size_t aligned_size   = heap_align_value_to_upper((uint32_t)size);
    uint32_t blocks_needed = aligned_size / HEAP_BLOCK_SIZE;

    status_t start_block = heap_malloc_blocks(heap, blocks_needed);
    if (status_is_error(start_block)) {
        return start_block;
    }

    void* addr = heap_block_to_address(heap, (uint32_t)start_block);

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