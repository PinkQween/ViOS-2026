#ifndef STDIO_H
#define STDIO_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file stdio.h
 * @brief User-space standard I/O declarations.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

/**
 * Write one character to standard output.
 *
 * @param c Character to write.
 * @return Written character.
 */
int putchar(int c);

/**
 * Read one character from standard input.
 *
 * @return Character value read from input.
 */
int getchar();

/**
 * Print formatted text to standard output.
 *
 * @param format printf-style format string.
 * @return Number of characters written.
 */
int printf(const char* format, ...);

#endif /* STDIO_H */
