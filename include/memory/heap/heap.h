#ifndef HEAP_H
#define HEAP_H

#include "config.h"
#include "status.h"

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
 * Free a pointer previously allocated from a specific heap instance.
 *
 * @param heap Heap descriptor.
 * @param ptr Allocation pointer to free.
 * @return None.
 */
void heap_free(struct heap* heap, void* ptr);

#endif /* HEAP_H */