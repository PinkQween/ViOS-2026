#ifndef STRING_H
#define STRING_H

#include <stddef.h>
#include <stdbool.h>

/**
 * Compute length of a null-terminated string.
 *
 * @param str Input string.
 * @return Number of characters before '\0'.
 */
size_t strlen(const char *str);

/**
 * Compute string length capped at a maximum count.
 *
 * @param str Input string.
 * @param maxlen Maximum characters to scan.
 * @return String length up to maxlen.
 */
size_t strnlen(const char *str, size_t maxlen);

/**
 * Check whether a character is an ASCII decimal digit.
 *
 * @param c Character to test.
 * @return true if c is in range '0'..'9', otherwise false.
 */
bool is_digit(char c);

/**
 * Convert an integer to a string representation.
 *
 * @param value Integer value to convert.
 * @param str Destination buffer.
 * @param base Numeric base (e.g. 10, 16).
 * @return Pointer to str.
 */
char *itoa(int value, char *str, int base);

/**
 * Print formatted text to kernel output.
 *
 * @param format Printf-style format string.
 * @return Number of characters written.
 */
int printf(const char *format, ...);

/**
 * Write formatted text into a bounded destination buffer.
 *
 * @param str Destination buffer.
 * @param size Total capacity of destination buffer.
 * @param format Printf-style format string.
 * @return Number of characters that would have been written.
 */
int snprintf(char *str, size_t size, const char *format, ...);

/**
 * Parse input text according to a scanf-like format.
 *
 * @param str Input string.
 * @param format Scanf-style format string.
 * @return Number of fields successfully parsed.
 */
int sscanf(const char *str, const char *format, ...);

/**
 * Copy a null-terminated source string into destination.
 *
 * @param dest Destination buffer.
 * @param src Source string.
 * @return Pointer to dest.
 */
char *strcpy(char *dest, const char *src);

/**
 * Copy up to n characters from source string to destination, null-terminating if possible.
 * 
 * @param dest Destination buffer.
 * @param src Source string.
 * @param n Maximum number of characters to copy from src.
 * @return Pointer to dest.
 */
char *strncpy(char *dest, const char *src, size_t n);

/**
 * Compare two strings up to n characters, case-sensitive.
 *
 * @param s1 First string.
 * @param s2 Second string.
 * @param n Maximum characters to compare.
 * @return 0 if equal within n chars, negative/positive on mismatch.
 */
int strncmp(const char *s1, const char *s2, size_t n);

/**
 * Compare two strings up to n characters, ASCII case-insensitive.
 *
 * @param s1 First string.
 * @param s2 Second string.
 * @param n Maximum characters to compare.
 * @return 0 if equal within n chars, negative/positive on mismatch.
 */
int istrncmp(const char *s1, const char *s2, size_t n);

/**
 * Compare two null-terminated strings.
 *
 * @param s1 First string.
 * @param s2 Second string.
 * @return 0 if equal, negative/positive on first mismatch.
 */
int strcmp(const char *s1, const char *s2);

/**
 * Count characters until terminator, null byte, or max.
 *
 * @param str Input string.
 * @param max Maximum characters to inspect.
 * @param terminator Sentinel terminator character.
 * @return Number of characters scanned before stop condition.
 */
int strlen_terminator(const char *str, int max, char terminator);

/**
 * Convert an ASCII character to lowercase.
 *
 * @param c Input character.
 * @return Lowercased character when in 'A'..'Z', otherwise unchanged.
 */
char tolower(char c);

#endif /* STRING_H */
