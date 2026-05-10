#ifndef HEAP_H
#define HEAP_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file heap.h
 * @brief Fixed-block heap allocator.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include "config.h"
#include "status.h"

#include "stdbool.h"
#include "stdint.h"
#include "stddef.h"

/** Heap table marker for an allocated block. */
#define HEAP_BLOCK_TABLE_ENTRY_TAKEN 1

/** Heap table marker for a free block. */
#define HEAP_BLOCK_TABLE_ENTRY_FREE 0

/** Allocation entry flag indicating this block chains to another block. */
#define HEAP_BLOCK_HAS_NEXT 0b10000000

/** Allocation entry flag indicating this is the first block of an allocation. */
#define HEAP_BLOCK_IS_FIRST  0b01000000

/** Encoded heap table entry value. */
typedef unsigned char HEAP_BLOCK_TABLE_ENTRY;

/**
 * Allocation hook called once for each block marked allocated.
 *
 * The callback receives the block address and block size. The return value is
 * currently ignored; callers should treat this as a notification hook.
 */
typedef void*(*HEAP_BLOCK_ALLOCATED_CALLBACK_FUNCTION)(void* ptr, size_t size);

/** Free hook called once for each block marked free. */
typedef void(*HEAP_BLOCK_FREE_CALLBACK_FUNCTION)(void* ptr);

/**
 * Heap table metadata used to track block ownership.
 */
typedef struct heap_table
{
    /** Table entries array (one entry per heap block). */
    HEAP_BLOCK_TABLE_ENTRY* entries;
    /** Total number of entries in the table. */
    size_t total;
} heap_table_t;

/**
 * Heap descriptor binding table metadata to an address range.
 */
struct heap
{
    struct heap_table* table;

    // Start address of the heap data pool
    void* start_address;

    // End address of the heap data pool
    void* end_address;

    size_t total_blocks;
    size_t free_blocks;
    size_t used_blocks;

    // Callback function for when a block is allocated
    HEAP_BLOCK_ALLOCATED_CALLBACK_FUNCTION block_allocated_callback;

    // Calback function for a when a block is freed.
    HEAP_BLOCK_FREE_CALLBACK_FUNCTION block_free_callback;
};

/**
 * Set block allocation/free notification callbacks for a heap.
 *
 * @param heap Heap descriptor to set callbacks for.
 * @param allocated_callback Optional callback invoked for each allocated block.
 * @param free_callback Optional callback invoked for each freed block.
 */
void heap_callbacks_set(struct heap* heap, HEAP_BLOCK_ALLOCATED_CALLBACK_FUNCTION allocated_callback, HEAP_BLOCK_FREE_CALLBACK_FUNCTION free_callback);

/**
 * Align a value up to the nearest multiple of the heap block size.
 * This is used to ensure that allocation sizes are rounded up to the nearest block size, since the heap operates on fixed-size blocks.
 *
 * @param value Value to align.
 * @return Aligned value rounded up to the nearest multiple of the heap block size.
 */
uintptr_t heap_align_value_to_upper(uintptr_t value);

/**
 * Create and validate a heap over a memory range.
 *
 * @param heap Heap descriptor to initialize.
 * @param ptr Start address of heap memory range.
 * @param end End address of heap memory range.
 * @param table Heap table metadata and backing entries.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t heap_create(struct heap* heap, void* ptr, void* end, struct heap_table* table);

size_t heap_address_to_block(struct heap* heap, void* address);

/**
 * Allocate memory from a specific heap instance.
 *
 * @param heap Heap descriptor.
 * @param size Requested allocation size in bytes.
 * @param out_ptr Output pointer for allocated memory.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t heap_malloc(struct heap* heap, size_t size, void** out_ptr);

/**
 * Allocate zero-initialized memory from a specific heap instance.
 *
 * This is a convenience wrapper around heap_malloc that zeroes the requested
 * allocation size on success.
 * 
 * @param heap Heap descriptor.
 * @param size Requested allocation size in bytes.
 * @return Pointer to zero-initialized memory, or NULL on error.
 */
void* heap_zalloc(struct heap* heap, size_t size);

/**
 * Free a pointer previously allocated from a specific heap instance.
 *
 * @param heap Heap descriptor.
 * @param ptr Allocation pointer to free.
 */
void heap_free(struct heap* heap, void* ptr);

/**
 * Get the total size of a heap.
 *
 * @param heap Heap descriptor.
 * @return Total size of the heap.
 */
size_t heap_total_size(struct heap* heap);

/**
 * Get the total used bytes in a heap.
 *
 * @param heap Heap descriptor.
 * @return Total used bytes in the heap.
 */
size_t heap_total_used(struct heap* heap);

/**
 * Check if a pointer is within the bounds of a heap.
 * 
 * @param heap Heap descriptor.
 * @param ptr Pointer to check.
 * @return true if the pointer is within the heap's address range, false otherwise.
 */
bool heap_is_address_within_heap(struct heap* heap, void* ptr);

/**
 * Count the number of contiguous allocated blocks starting from a given address.
 * This is used to determine the size of an allocation when freeing, since only the
 * starting address of an allocation is stored in the heap table.
 * 
 * @param heap Heap descriptor.
 * @param starting_address Starting address of the allocation to count blocks for.
 * @return Number of contiguous allocated blocks in the allocation, or 0 if the starting address is invalid or not allocated.
 */
size_t heap_allocation_block_count(struct heap* heap, void* starting_address);

void* heap_realloc(struct heap* heap, void* old_ptr, size_t new_size);

#endif /* HEAP_H */
