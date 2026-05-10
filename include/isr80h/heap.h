#ifndef ISR80H_HEAP_H
#define ISR80H_HEAP_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file heap.h
 * @brief ISR 0x80 heap system call handlers.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

struct interrupt_frame;

/**
 * Handle SYSTEM_COMMAND3_MALLOC.
 *
 * @param frame Saved interrupt frame containing syscall arguments.
 * @return Allocated user pointer, or NULL on failure.
 */
void* isr80h_command3_malloc(struct interrupt_frame* frame);

/**
 * Handle SYSTEM_COMMAND4_FREE.
 *
 * @param frame Saved interrupt frame containing syscall arguments.
 * @return Command result pointer, or NULL.
 */
void* isr80h_command4_free(struct interrupt_frame* frame);

#endif /* ISR80H_HEAP_H */
