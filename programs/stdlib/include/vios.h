#ifndef VIOS_H
#define VIOS_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file vios.h
 * @brief ViOS user-space system call wrapper API.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include "stdint.h"
#include "stdbool.h"

struct command_argument
{
    char argument[4096];
    struct command_argument* next;
};

struct process_arguments
{
    char** argv;
    int argc;
};

/**
 * Print a null-terminated string through the ViOS syscall interface.
 *
 * @param str Text to print.
 * @return None.
 */
void vios_print(const char* str);

/**
 * Read one key through the ViOS syscall interface.
 *
 * @return Key value.
 */
int vios_getkey();

/**
 * Print one character through the ViOS syscall interface.
 *
 * @param c Character to print.
 * @return None.
 */
void vios_putchar(char c);

/**
 * Allocate memory through the ViOS process heap syscall.
 *
 * @param size Number of bytes to allocate.
 * @return Allocated pointer, or NULL on failure.
 */
void* vios_malloc(size_t size);

/**
 * Free memory allocated through vios_malloc.
 *
 * @param ptr Pointer to free.
 * @return None.
 */
void vios_free(void* ptr);

/**
 * Read a terminal line into a caller-provided buffer.
 *
 * @param buffer Destination buffer.
 * @param max_length Maximum number of bytes to store.
 * @param echo Whether typed characters should be echoed.
 * @return None.
 */
void vios_terminal_readline(char* buffer, int max_length, bool echo);

/**
 * Load and start a process by filename.
 *
 * @param filename Program filename to execute.
 * @return None.
 */
void vios_process_load_start(const char* filename);

/**
 * Invoke a shell/system command from parsed arguments.
 *
 * @param args Command argument list.
 * @return Command status code.
 */
int vios_invoke_system_command(struct command_argument* args);

/**
 * Retrieve arguments for the current process.
 *
 * @param args Output structure receiving argc and argv.
 * @return None.
 */
void vios_process_get_arguments(struct process_arguments* args);

/**
 * Parse and run a command string.
 *
 * @param command Command line to execute.
 * @return Command status code.
 */
int vios_system_run(const char* command);

/**
 * Exit the current process.
 *
 * @param exit_code Process exit code.
 * @return None.
 */
void vios_exit(int exit_code);

#endif
