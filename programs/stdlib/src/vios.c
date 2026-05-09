#include "vios.h"
#include "string.h"
#include "errno.h"

int vios_getkey_blocking()
{
    int key;
    do {
        key = vios_getkey();
    } while (key == 0);
    return key;
}

void vios_terminal_readline(char* buffer, int max_length, bool echo)
{
    int index = 0;

    if (!buffer || max_length <= 0) {
        return;
    }
    
    while (index < max_length - 1) {
        char key = (char)vios_getkey_blocking();

        if (key == '\r')
        {
            break;
        }

        if (key == '\b') {
            if (index > 0) {
                index--;

                if (echo) {
                    vios_putchar(key);
                }
            }

            continue;
        }

        buffer[index++] = key;

        if (echo) {
            vios_putchar(key);
        }
    }

    buffer[index] = '\0';
}

struct command_argument* vios_parse_command(const char* command, int max)
{
    struct command_argument* root_command = 0;
    struct command_argument* current = 0;
    int index = 0;

    if (!command || max <= 0) {
        return 0;
    }

    if (max > 4096) {
        return 0;
    }

    while (index < max && command[index]) {
        struct command_argument* new_arg;
        int argument_index = 0;
        bool in_quotes = false;

        while (index < max && command[index] == ' ') {
            index++;
        }

        if (index >= max || !command[index]) {
            break;
        }

        new_arg = vios_malloc(sizeof(struct command_argument));
        if (!new_arg) {
            struct command_argument* temp = root_command;
            while (temp) {
                struct command_argument* next = temp->next;
                vios_free(temp);
                temp = next;
            }
            return 0;
        }

        while (index < max && command[index]) {
            char c = command[index];

            if (c == '"') {
                in_quotes = !in_quotes;
                index++;
                continue;
            }

            if (!in_quotes && c == ' ') {
                break;
            }

            if (argument_index < (int)sizeof(new_arg->argument) - 1) {
                new_arg->argument[argument_index++] = c;
            }

            index++;
        }

        new_arg->argument[argument_index] = '\0';
        new_arg->next = 0;

        if (!root_command) {
            root_command = new_arg;
            current = new_arg;
        } else {
            current->next = new_arg;
            current = new_arg;
        }

        while (index < max && command[index] == ' ') {
            index++;
        }
    }

    return root_command;
}

int vios_system_run(const char* command)
{
    char buffer[4096];
    strncpy(buffer, command, sizeof(buffer));
    struct command_argument* args = vios_parse_command(buffer, sizeof(buffer));

    if (!args) {
        return EINVAL;
    }

    return vios_invoke_system_command(args);
}
