#ifndef ISR80H_PROCESS_H
#define ISR80H_PROCESS_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file process.h
 * @brief ISR 0x80 process system call handlers.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

struct interrupt_frame;

/**
 * Handle SYSTEM_COMMAND5_PROCESS_LOAD_START.
 *
 * @param frame Saved interrupt frame containing syscall arguments.
 * @return Command result pointer, or NULL.
 */
void* isr80h_command5_process_load_start(struct interrupt_frame* frame);

/**
 * Handle SYSTEM_COMMAND6_INVOKE_SYSTEM_COMMAND.
 *
 * @param frame Saved interrupt frame containing syscall arguments.
 * @return Encoded command result cast through a pointer-sized result.
 */
void* isr80h_command6_invoke_system_command(struct interrupt_frame* frame);

/**
 * Handle SYSTEM_COMMAND7_GET_PROCESS_ARGUMENTS.
 *
 * @param frame Saved interrupt frame containing syscall arguments.
 * @return Command result pointer, or NULL.
 */
void* isr80h_command7_get_process_arguments(struct interrupt_frame* frame);

/**
 * Handle SYSTEM_COMMAND8_EXIT.
 *
 * @param frame Saved interrupt frame containing syscall arguments.
 * @return Command result pointer, or NULL.
 */
void* isr80h_command8_exit(struct interrupt_frame* frame);

#endif /* ISR80H_PROCESS_H */
