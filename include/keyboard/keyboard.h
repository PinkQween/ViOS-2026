#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "status.h"

struct process;


typedef int(*KEYBOARD_INIT_FUNCTION)(void);

struct keyboard
{
    KEYBOARD_INIT_FUNCTION init;

    char name[32];
    struct keyboard* next;
};

/**
 * Initializes the keyboard subsystem, setting up any necessary data structures and initializing all registered keyboards. This function should be called during system initialization to ensure that the keyboard is ready for use when the system starts running.
 *
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t keyboard_init();

/**
 * Handles a backspace input from the keyboard for the given process, removing the last character from the process's keyboard buffer.
 * 
 * @param process The process whose keyboard buffer should be modified.
 */
void keyboard_backspace(struct process* process);

/**
 * Pushes a character into the current process's keyboard buffer, allowing it to be read by the process when it retrieves input from the keyboard.
 * 
 * @param c The character to push into the keyboard buffer.
 */
void keyboard_push(char c);

/**
 * Pops a character from the current process's keyboard buffer, returning it to the caller. This function is used to retrieve input from the keyboard for the currently running process.
 * 
 * @return The character popped from the keyboard buffer, or 0 if the buffer is empty or no process is currently running.
 */
char keyboard_pop();

#endif /* KEYBOARD_H */
