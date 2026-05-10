#ifndef HEAP_H
#define HEAP_H

#include "config.h"
#include "status.h"

#include "stdint.h"
#include "stddef.h"

/**
 * @file heap.h
 * @brief Heap allocator definitions and functions.
 * This header defines the structures and functions for managing a heap
 * allocator in an x86_64 operating system kernel. The heap allocator
 * provides dynamic memory management for the kernel, allowing it to
 * allocate and free memory at runtime.
 * The heap is implemented as a simple block-based allocator that manages
 * a contiguous region of memory. It uses a table to track the status of
 * each block (free or taken) and supports allocating multiple contiguous
 * blocks for larger allocation requests.
 * The heap allocator includes functions for creating and validating a heap,
 * allocating memory, and freeing allocated memory. It also provides
 * helper functions for aligning allocation sizes and converting between
 * block indices and memory addresses.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

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
    /** Allocation tracking table. */
    struct heap_table table;
    /** Base address of managed heap memory region. */
    void* start_address;
    void* end_address;
};

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
 * This is a convenience wrapper around heap_malloc that also zeroes the allocated memory.
 * 
 * @param heap Heap descriptor.
 * @param size Requested allocation size in bytes.
 * @param out_ptr Output pointer for allocated and zero-initialized memory.
 * @return STATUS_OK on success, negative status_t on error.
 */
void* heap_zalloc(struct heap* heap, size_t size);

/**
 * Free a pointer previously allocated from a specific heap instance.
 *
 * @param heap Heap descriptor.
 * @param ptr Allocation pointer to free.
 * @return None.
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
bool heap_is_address_in_heap(struct heap* heap, void* ptr);

#endif /* HEAP_H */
