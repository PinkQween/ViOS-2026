#ifndef ISR80H_IO_H
#define ISR80H_IO_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file io.h
 * @brief ISR 0x80 console and keyboard system call handlers.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

struct interrupt_frame;

/**
 * Handle SYSTEM_COMMAND0_PRINT.
 *
 * @param frame Saved interrupt frame containing syscall arguments.
 * @return Command result pointer, or NULL.
 */
void* isr80h_command0_print(struct interrupt_frame* frame);

/**
 * Handle SYSTEM_COMMAND1_GETKEY.
 *
 * @param frame Saved interrupt frame containing syscall arguments.
 * @return Encoded key value cast through a pointer-sized result.
 */
void* isr80h_command1_getkey(struct interrupt_frame* frame);

/**
 * Handle SYSTEM_COMMAND2_PUTCHAR.
 *
 * @param frame Saved interrupt frame containing syscall arguments.
 * @return Command result pointer, or NULL.
 */
void* isr80h_command2_putchar(struct interrupt_frame* frame);

#endif /* ISR80H_IO_H */
