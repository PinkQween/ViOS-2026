#ifndef STRING_H
#define STRING_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file string.h
 * @brief User-space string and memory utility declarations.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include "stddef.h"

/**
 * Compare two null-terminated strings.
 *
 * @param s1 First string.
 * @param s2 Second string.
 * @return Negative, zero, or positive value based on lexical ordering.
 */
int strcmp(const char* s1, const char* s2);

/**
 * Compute the length of a null-terminated string.
 *
 * @param str Input string.
 * @return Number of characters before the null terminator.
 */
int strlen(const char* str);

/**
 * Copy bytes from one memory region to another.
 *
 * @param dest Destination memory region.
 * @param src Source memory region.
 * @param n Number of bytes to copy.
 * @return Pointer to dest.
 */
void* memcpy(void* dest, const void* src, size_t n);

/**
 * Fill a memory region with a byte value.
 *
 * @param dest Destination memory region.
 * @param val Byte value to write.
 * @param n Number of bytes to set.
 * @return Pointer to dest.
 */
void* memset(void* dest, int val, size_t n);

/**
 * Copy a null-terminated string.
 *
 * @param dest Destination buffer.
 * @param src Source string.
 * @return Pointer to dest.
 */
char* strcpy(char* dest, const char* src);

/**
 * Split a string into delimiter-separated tokens.
 *
 * @param str String to tokenize, or NULL to continue the previous string.
 * @param delim Delimiter characters.
 * @return Next token, or NULL when no tokens remain.
 */
char* strtok(char* str, const char* delim);

/**
 * Copy at most n characters from a string.
 *
 * @param dest Destination buffer.
 * @param src Source string.
 * @param n Maximum number of characters to copy.
 * @return Pointer to dest.
 */
char* strncpy(char* dest, const char* src, size_t n);

#endif /* STRING_H */
