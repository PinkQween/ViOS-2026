#ifndef IO_H
#define IO_H

#include <stdint.h>

/**
 * @file io.h
 * @brief Low-level x86 port I/O interface.
 * 
 * This header provides functions to perform single and string-based
 * input/output operations on x86 I/O ports. These functions are
 * intended for use in kernel development or bare-metal programming.
 * 
 * All functions assume that the caller has the necessary privilege
 * level to access hardware ports (usually ring 0).
 * 
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

/* =========================================================
 * STRING INPUT FUNCTIONS (port -> memory)
 * ========================================================= */

/**
 * @brief Read count bytes from an IO port into a memory buffer.
 *
 * @param port IO port address.
 * @param buffer Destination memory buffer.
 * @param count Number of bytes to read.
 */
void insb(uint16_t port, void *buffer, uint32_t count);

/**
 * @brief Read count 16-bit words from an IO port into a memory buffer.
 *
 * @param port IO port address.
 * @param buffer Destination memory buffer.
 * @param count Number of words to read.
 */
void insw(uint16_t port, void *buffer, uint32_t count);

/**
 * @brief Read count 32-bit double-words from an IO port into a memory buffer.
 *
 * @param port IO port address.
 * @param buffer Destination memory buffer.
 * @param count Number of double-words to read.
 */
void insd(uint16_t port, void *buffer, uint32_t count);

/* =========================================================
 * STRING OUTPUT FUNCTIONS (memory -> port)
 * ========================================================= */

/**
 * @brief Write count bytes from a memory buffer to an IO port.
 *
 * @param port IO port address.
 * @param buffer Source memory buffer.
 * @param count Number of bytes to write.
 */
void outsb(uint16_t port, const void *buffer, uint32_t count);

/**
 * @brief Write count 16-bit words from a memory buffer to an IO port.
 *
 * @param port IO port address.
 * @param buffer Source memory buffer.
 * @param count Number of words to write.
 */
void outsw(uint16_t port, const void *buffer, uint32_t count);

/**
 * @brief Write count 32-bit double-words from a memory buffer to an IO port.
 *
 * @param port IO port address.
 * @param buffer Source memory buffer.
 * @param count Number of double-words to write.
 */
void outsd(uint16_t port, const void *buffer, uint32_t count);

/* =========================================================
 * SINGLE PORT I/O FUNCTIONS
 * ========================================================= */

/**
 * @brief Read a single byte from an IO port.
 *
 * @param port IO port address.
 * @return 8-bit value read from the port.
 */
uint8_t inb(uint16_t port);

/**
 * @brief Read a single 16-bit word from an IO port.
 *
 * @param port IO port address.
 * @return 16-bit value read from the port.
 */
uint16_t inw(uint16_t port);

/**
 * @brief Read a single 32-bit double-word from an IO port.
 *
 * @param port IO port address.
 * @return 32-bit value read from the port.
 */
uint32_t ind(uint16_t port);

/**
 * @brief Write a single byte to an IO port.
 *
 * @param port IO port address.
 * @param value 8-bit value to write.
 */
void outb(uint16_t port, uint8_t value);

/**
 * @brief Write a single 16-bit word to an IO port.
 *
 * @param port IO port address.
 * @param value 16-bit value to write.
 */
void outw(uint16_t port, uint16_t value);

/**
 * @brief Write a single 32-bit double-word to an IO port.
 *
 * @param port IO port address.
 * @param value 32-bit value to write.
 */
void outd(uint16_t port, uint32_t value);

#endif /* IO_H */