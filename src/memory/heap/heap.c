#include "memory/heap/heap.h"
#include "status.h"
#include "memory/memory.h"

#include "stdbool.h"

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

static status_t heap_validate_table(void* start, void* end, struct heap_table* table)
{
    if (!table->entries || (uintptr_t)end <= (uintptr_t)start) {
        return STATUS_ERR(EINVAL);
    }

    size_t total_size   = (size_t)((uintptr_t)end - (uintptr_t)start);
    size_t total_blocks = total_size / HEAP_BLOCK_SIZE;

    if (total_size < HEAP_MINIMUM_SIZE_BYTES) {
        return STATUS_ERR(EINVAL);
    }

    if (table->total != total_blocks) {
        return STATUS_ERR(EINVAL);
    }

    return STATUS_OK;
}

static bool heap_address_is_block_aligned(void* ptr)
{
    return ((uintptr_t)ptr % HEAP_BLOCK_SIZE) == 0;
}

void heap_callbacks_set(struct heap* heap, HEAP_BLOCK_ALLOCATED_CALLBACK_FUNCTION allocated_callback, HEAP_BLOCK_FREE_CALLBACK_FUNCTION free_callback)
{
    if (!heap) {
        return;
    }

    heap->block_allocated_callback = allocated_callback;
    heap->block_free_callback = free_callback;
}

status_t heap_create(struct heap* heap, void* start, void* end, struct heap_table* table)
{
    if (!heap || !start || !end || !table) {
        return STATUS_ERR(EINVAL);
    }

    if (!heap_address_is_block_aligned(start) || !heap_address_is_block_aligned(end)) {
        return STATUS_ERR(EINVAL);
    }

    memset(heap, 0, sizeof(*heap));

    heap->start_address = start;
    heap->end_address = end;
    heap->table = table;
    heap->total_blocks = table->total;
    heap->free_blocks = table->total;
    heap->used_blocks = 0;

    status_t res = heap_validate_table(start, end, table);
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

static int heap_table_entry_type(HEAP_BLOCK_TABLE_ENTRY entry)
{
    return entry & 0x0F;
}

static status_t heap_find_free_block_run(struct heap* heap, size_t blocks_needed)
{
    struct heap_table* table = heap->table;

    size_t free_blocks_seen = 0;
    int64_t first_free_block = -1;

    for (size_t i = 0; i < table->total; i++) {
        if (heap_table_entry_type(table->entries[i]) == HEAP_BLOCK_TABLE_ENTRY_FREE) {
            if (first_free_block == -1) {
                first_free_block = (int64_t)i;
            }

            free_blocks_seen++;

            if (free_blocks_seen == blocks_needed) {
                return (status_t)first_free_block;
            }
        } else {
            first_free_block = -1;
            free_blocks_seen = 0;
        }
    }

    return STATUS_ERR(ENOMEM);
}

size_t heap_allocation_block_count(struct heap* heap, void* starting_address)
{
    size_t count = 0;
    struct heap_table* heap_table = heap->table;
    int64_t starting_block = heap_address_to_block(heap, starting_address);

    if (starting_block < 0)
    {
        goto out;
    }

    for (int64_t i = starting_block; i < (int64_t) heap_table->total; i++)
    {
        HEAP_BLOCK_TABLE_ENTRY entry = heap_table->entries[i];
        if (entry & HEAP_BLOCK_TABLE_ENTRY_TAKEN)
        {
            count++;
        }

        if (!(entry & HEAP_BLOCK_HAS_NEXT))
        {
            break;
        }
    }

out:
    return count;
}

static void* heap_block_to_address(struct heap* heap, size_t block)
{
    return (void*)(
        (uintptr_t)heap->start_address +
        (block * HEAP_BLOCK_SIZE)
    );
}

static void heap_mark_block_run_allocated(struct heap* heap, size_t start_block, size_t blocks_needed)
{
    size_t end_block = start_block + blocks_needed - 1;

    HEAP_BLOCK_TABLE_ENTRY entry =
        HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FIRST;

    if (blocks_needed > 1) {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    for (size_t i = start_block; i <= end_block; i++) {
        heap->table->entries[i] = entry;

        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;

        if (i != end_block) {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }

        void* address = heap_block_to_address(heap, i);
        
        if (heap->block_allocated_callback) {
            heap->block_allocated_callback(address, HEAP_BLOCK_SIZE);
        }
    }
}

static void heap_mark_block_run_free(struct heap* heap, size_t start_block)
{
    struct heap_table* table = heap->table;

    for (size_t i = start_block; i < table->total; i++) {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[i];
        table->entries[i] = HEAP_BLOCK_TABLE_ENTRY_FREE;

        void* address = heap_block_to_address(heap, i);
        
        if (heap->block_free_callback) {
            heap->block_free_callback(address);
        }

        if (!(entry & HEAP_BLOCK_HAS_NEXT)) {
            break;
        }
    }
}

static status_t heap_allocate_block_run(struct heap* heap, size_t blocks_needed)
{
    if (!heap) {
        return STATUS_ERR(EINVAL);
    }

    status_t start_block = heap_find_free_block_run(heap, blocks_needed);
    if (status_is_error(start_block)) {
        return start_block;
    }

    heap_mark_block_run_allocated(heap, (size_t)start_block, blocks_needed);

    return start_block;
}

size_t heap_address_to_block(struct heap *heap, void *address)
{
    return (
        (uintptr_t)address -
        (uintptr_t)heap->start_address
    ) / HEAP_BLOCK_SIZE;
}
status_t heap_malloc(struct heap* heap, size_t size, void** out_ptr)
{
    if (!heap || size == 0) {
        return STATUS_ERR(EINVAL);
    }

    size_t aligned_size   = heap_align_value_to_upper(size);
    size_t blocks_needed = aligned_size / HEAP_BLOCK_SIZE;

    status_t start_block = heap_allocate_block_run(heap, blocks_needed);
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

    if (!heap_is_address_within_heap(heap, ptr) || !heap_address_is_block_aligned(ptr)) {
        return;
    }

    size_t block = (size_t)heap_address_to_block(heap, ptr);
    HEAP_BLOCK_TABLE_ENTRY entry = heap->table->entries[block];

    if (heap_table_entry_type(entry) != HEAP_BLOCK_TABLE_ENTRY_TAKEN ||
        !(entry & HEAP_BLOCK_IS_FIRST)) {
        return;
    }

    heap_mark_block_run_free(heap, block);
}

size_t heap_total_size(struct heap* heap)
{
    return heap->table->total * HEAP_BLOCK_SIZE;
}

size_t heap_total_used(struct heap* heap)
{
    size_t used_blocks = 0;

    for (size_t i = 0; i < heap->table->total; i++) {
        if (heap_table_entry_type(heap->table->entries[i]) == HEAP_BLOCK_TABLE_ENTRY_TAKEN) {
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

bool heap_is_address_within_heap(struct heap* heap, void* ptr)
{
    return (uintptr_t)ptr >= (uintptr_t)heap->start_address &&
           (uintptr_t)ptr < (uintptr_t)heap->end_address;
}

static bool heap_is_block_range_free(
    struct heap* heap,
    size_t starting_block,
    size_t ending_block)
{
    if (!heap || ending_block >= heap->table->total)
    {
        return false;
    }

    for (size_t i = starting_block; i <= ending_block; i++)
    {
        if (heap->table->entries[i] & HEAP_BLOCK_TABLE_ENTRY_TAKEN)
        {
            return false;
        }
    }

    return true;
}

void* heap_realloc(struct heap* heap, void* old_ptr, size_t new_size)
{
    if (!heap)
    {
        return NULL;
    }

    // realloc(NULL, size) == malloc(size)
    if (!old_ptr)
    {
        if (new_size == 0)
        {
            return NULL;
        }

        heap_malloc(heap, new_size, &old_ptr);
        return old_ptr;
    }

    // realloc(ptr, 0) == free(ptr)
    if (new_size == 0)
    {
        heap_free(heap, old_ptr);
        return NULL;
    }

    // Validate pointer
    if (!heap_is_address_within_heap(heap, old_ptr))
    {
        return NULL;
    }

    int64_t starting_block = heap_address_to_block(heap, old_ptr);

    if (starting_block < 0 ||
        starting_block >= (int64_t)heap->table->total)
    {
        return NULL;
    }

    size_t current_alloc_blocks =
        heap_allocation_block_count(heap, old_ptr);

    if (current_alloc_blocks == 0)
    {
        return NULL;
    }

    int64_t ending_block =
        starting_block + current_alloc_blocks - 1;

    size_t new_size_aligned =
        heap_align_value_to_upper(new_size);

    size_t new_total_blocks =
        new_size_aligned / HEAP_BLOCK_SIZE;

    size_t old_total_size =
        current_alloc_blocks * HEAP_BLOCK_SIZE;

    //
    // SHRINK
    //
    if (new_total_blocks <= current_alloc_blocks)
    {
        if (new_total_blocks == current_alloc_blocks)
        {
            return old_ptr;
        }

        int64_t block_to_free =
            starting_block + new_total_blocks;

        // This already updates heap counters
        heap_mark_block_run_free(heap, block_to_free);

        // Mark new last block correctly
        if (new_total_blocks > 0)
        {
            heap->table->entries
                [starting_block + new_total_blocks - 1]
                    &= ~HEAP_BLOCK_HAS_NEXT;
        }

        return old_ptr;
    }

    //
    // EXPAND IN PLACE
    //
    size_t extra_blocks =
        new_total_blocks - current_alloc_blocks;

    int64_t extension_start = ending_block + 1;
    int64_t extension_end =
        extension_start + extra_blocks - 1;

    // Bounds check
    if (extension_end < (int64_t)heap->table->total)
    {
        if (heap_is_block_range_free(
                heap,
                extension_start,
                extension_end))
        {
            for (int64_t i = extension_start;
                 i <= extension_end;
                 i++)
            {
                HEAP_BLOCK_TABLE_ENTRY entry =
                    HEAP_BLOCK_TABLE_ENTRY_TAKEN;

                if (i != extension_end)
                {
                    entry |= HEAP_BLOCK_HAS_NEXT;
                }

                heap->table->entries[i] = entry;

                if (heap->block_allocated_callback)
                {
                    heap->block_allocated_callback(
                        heap_block_to_address(heap, i),
                        HEAP_BLOCK_SIZE);
                }
            }

            // Previous end now points forward
            heap->table->entries[ending_block]
                |= HEAP_BLOCK_HAS_NEXT;

            heap->used_blocks += extra_blocks;
            heap->free_blocks -= extra_blocks;

            return old_ptr;
        }
    }

    //
    // FALLBACK: allocate new block
    //
    void* new_addr = 0;
    heap_malloc(heap, new_size_aligned, &new_addr);

    if (!new_addr)
    {
        return NULL;
    }

    size_t copy_size = old_total_size;

    if (copy_size > new_size)
    {
        copy_size = new_size;
    }

    memcpy(new_addr, old_ptr, copy_size);

    heap_free(heap, old_ptr);

    return new_addr;
}