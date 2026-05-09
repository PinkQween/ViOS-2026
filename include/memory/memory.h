#ifndef MEMORY_H
#define MEMORY_H

#include "stddef.h"
#include "stdbool.h"

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

#endif /* MEMORY_H */
