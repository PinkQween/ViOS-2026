#ifndef IO_H
#define IO_H

#include "stdint.h"

/**
 * Read count bytes from IO port into a memory buffer.
 *
 * @param port IO port address.
 * @param buffer Destination memory buffer.
 * @param count Number of bytes to read.
 * @return None.
 */
void insb(uint16_t port, void *buffer, uint32_t count);

/**
 * Read count words from IO port into a memory buffer.
 *
 * @param port IO port address.
 * @param buffer Destination memory buffer.
 * @param count Number of 16-bit words to read.
 * @return None.
 */
void insw(uint16_t port, void *buffer, uint32_t count);

/**
 * Read count double-words from IO port into a memory buffer.
 *
 * @param port IO port address.
 * @param buffer Destination memory buffer.
 * @param count Number of 32-bit values to read.
 * @return None.
 */
void insd(uint16_t port, void *buffer, uint32_t count);

/**
 * Write count bytes from memory buffer to IO port.
 *
 * @param port IO port address.
 * @param buffer Source memory buffer.
 * @param count Number of bytes to write.
 * @return None.
 */
void outsb(uint16_t port, const void *buffer, uint32_t count);

/**
 * Write count words from memory buffer to IO port.
 *
 * @param port IO port address.
 * @param buffer Source memory buffer.
 * @param count Number of 16-bit words to write.
 * @return None.
 */
void outsw(uint16_t port, const void *buffer, uint32_t count);

/**
 * Write count double-words from memory buffer to IO port.
 *
 * @param port IO port address.
 * @param buffer Source memory buffer.
 * @param count Number of 32-bit values to write.
 * @return None.
 */
void outsd(uint16_t port, const void *buffer, uint32_t count);

/**
 * Read one byte from IO port.
 *
 * @param port IO port address.
 * @return 8-bit value read from port.
 */
uint8_t  inb(uint16_t port);

/**
 * Read one 16-bit word from IO port.
 *
 * @param port IO port address.
 * @return 16-bit value read from port.
 */
uint16_t inw(uint16_t port);

/**
 * Read one 32-bit double-word from IO port.
 *
 * @param port IO port address.
 * @return 32-bit value read from port.
 */
uint32_t ind(uint16_t port);

/**
 * Write one byte to IO port.
 *
 * @param port IO port address.
 * @param value 8-bit value to write.
 * @return None.
 */
void outb(uint16_t port, uint8_t value);

/**
 * Write one 16-bit word to IO port.
 *
 * @param port IO port address.
 * @param value 16-bit value to write.
 * @return None.
 */
void outw(uint16_t port, uint16_t value);

/**
 * Write one 32-bit double-word to IO port.
 *
 * @param port IO port address.
 * @param value 32-bit value to write.
 * @return None.
 */
void outd(uint16_t port, uint32_t value);

#endif /* IO_H */