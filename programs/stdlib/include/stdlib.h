#ifndef STDLIB_H
#define STDLIB_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file stdlib.h
 * @brief User-space allocation and conversion declarations.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include "stdint.h"

/**
 * Allocate memory from the process heap.
 *
 * @param size Number of bytes to allocate.
 * @return Allocated pointer, or NULL on failure.
 */
void* malloc(size_t size);

/**
 * Free memory previously returned by malloc.
 *
 * @param ptr Pointer to free.
 * @return None.
 */
void free(void* ptr);

/**
 * Convert an integer to a string in the requested base.
 *
 * @param i Integer value to convert.
 * @param str Destination buffer.
 * @param base Numeric base to use.
 * @return Pointer to str.
 */
char* itoa(int i, char* str, int base);

#endif /* STDLIB_H */
