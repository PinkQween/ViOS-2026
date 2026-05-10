#ifndef MEMORY_H
#define MEMORY_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file memory.h
 * @brief Kernel memory map and memory utility functions.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include "stddef.h"
#include "stdbool.h"
#include "stdint.h"

struct e820_entry
{
    uint64_t base_addr;
    uint64_t length;
    uint32_t type;
    uint32_t acpi_attrs;
} __attribute__((packed));

/**
 * Find the largest usable E820 memory map entry.
 *
 * @param entries E820 memory map entries.
 * @param total_entries Number of entries in the map.
 * @return Pointer to the largest usable entry, or NULL if none exists.
 */
struct e820_entry* e820_largest_free_entry(struct e820_entry* entries, size_t total_entries);

/**
 * Count all bytes marked usable in an E820 memory map.
 *
 * @param entries E820 memory map entries.
 * @param total_entries Number of entries in the map.
 * @return Total usable memory in bytes.
 */
size_t e820_total_accessible_memory(struct e820_entry* entries, size_t total_entries);

/**
 * Fill a memory region with a byte value.
 *
 * @param dest Destination memory region.
 * @param value Byte value to write.
 * @param count Number of bytes to set.
 * @return Pointer to dest.
 */
void *memset(void *dest, int value, size_t count);

/**
 * Copy bytes from source to destination.
 *
 * @param dest Destination memory region.
 * @param src Source memory region.
 * @param count Number of bytes to copy.
 * @return Pointer to dest.
 */
void *memcpy(void *dest, const void *src, size_t count);

/**
 * Copy bytes into a bounded destination region.
 *
 * @param dest Destination memory region.
 * @param dest_size Total capacity of destination memory region.
 * @param src Source memory region.
 * @param count Number of bytes to copy.
 * @return true if copied, false if arguments are invalid or count exceeds dest_size.
 */
bool safe_memcpy(void *dest, size_t dest_size, const void *src, size_t count);

/**
 * Compare two memory regions byte-by-byte.
 *
 * @param ptr1 First memory region.
 * @param ptr2 Second memory region.
 * @param count Number of bytes to compare.
 * @return 0 if equal, <0 if ptr1 < ptr2, >0 if ptr1 > ptr2.
 */
int memcmp(const void *ptr1, const void *ptr2, size_t count);

/**
 * Get toal entries in the E820 memory map.
 * 
 * @return Total number of entries in the E820 memory map.
 */
size_t e820_total_entries();

/**
 * Get a e820 entry
 * 
 * @param index Index of the entry to retrieve.
 * @return Pointer to the e820 entry at the specified index, or NULL if index is out of bounds.
 */
struct e820_entry* e820_get_entry(size_t index);

/**
 * Copy bytes from source to destination, handling overlapping regions safely.
 *
 * @param dest Destination memory region.
 * @param src Source memory region.
 * @param count Number of bytes to copy.
 * @return Pointer to dest.
 */
void* memmove(void* dest, const void* src, size_t count);

#endif /* MEMORY_H */
