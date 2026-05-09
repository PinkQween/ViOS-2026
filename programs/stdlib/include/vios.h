#ifndef VIOS_H
#define VIOS_H

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

void vios_print(const char* str);
int vios_getkey();
void vios_putchar(char c);
void* vios_malloc(size_t size);
void vios_free(void* ptr);
void vios_terminal_readline(char* buffer, int max_length, bool echo);
void vios_process_load_start(const char* filename);
int vios_invoke_system_command(struct command_argument* args);
void vios_process_get_arguments(struct process_arguments* args);
int vios_system_run(const char* command);
void vios_exit(int exit_code);

#endif