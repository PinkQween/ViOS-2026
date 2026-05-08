#ifndef PROCESS_H
#define PROCESS_H

#include "task.h"
#include "config.h"
#include "status.h"
#include "loader/elf/elfloader.h"

#include <stdint.h>
#include <stddef.h>
#include "keyboard/keyboard.h"  // For KEYBOARD_BUFFER_SIZE
#include "fs/file.h"   // For MAX_PATH_LENGTH

#define PROCESS_TYPE_ELF    0
#define PROCESS_TYPE_BINARY 1

typedef unsigned char PROCESS_FILE_TYPE;

struct keyboard_buffer
{
    char buffer[KEYBOARD_BUFFER_SIZE];
    int tail;
    int head;
};

struct process
{
    uint32_t pid;                        // Process ID
    char filename[MAX_PATH_LENGTH];       // Executable filename
    struct task* main_thread;            // Main task/thread of the process
    void* allocations[MAX_PROGRAM_ALLOCATIONS]; // Dynamic allocations tracked by the process
    PROCESS_FILE_TYPE filetype;          // Type of process: ELF or Binary

    union
    {
        void* ptr;                       // Raw binary pointer
        struct elf_file* elf;            // ELF file pointer
    };

    void* stack;                          // User-space stack pointer
    uint32_t size;                        // Size of the loaded binary
    struct keyboard_buffer keyboard;      // Keyboard input buffer
};

/*-----------------------------------------------------------------------------
| Process Management
-----------------------------------------------------------------------------*/

/**
 * Loads a user-space process from a specified filename and creates a new process structure.
 * Allocates a task for the process but does not switch to it.
 *
 * @param filename The path to the binary or ELF file.
 * @param process Out parameter for the newly created process pointer.
 * @return STATUS_OK on success, or an error code on failure.
 */
status_t process_load(const char* filename, struct process** process);

/**
 * Loads a process into a specific process slot.
 *
 * @param filename The path to the binary or ELF file.
 * @param process Out parameter for the newly created process pointer.
 * @param process_slot Slot index (PID) to load the process into.
 * @return STATUS_OK on success, or an error code on failure.
 */
status_t process_load_for_slot(const char* filename, struct process** process, int process_slot);

/**
 * Returns the currently running process.
 *
 * @return Pointer to the current process.
 */
struct process* process_current();

/**
 * Returns a process by its PID.
 *
 * @param pid The process ID to retrieve.
 * @return Pointer to the process, or NULL if not found.
 */
struct process* process_get(int pid);

/**
 * Switches the CPU execution to the given process.
 *
 * @param process Process to switch to.
 * @return STATUS_OK on success, or an error code on failure.
 */
status_t process_switch(struct process* process);

/**
 * Loads a process from a file and immediately switches execution to it.
 *
 * @param filename Path to the process file.
 * @param process Out parameter for the process pointer.
 * @return STATUS_OK on success, or an error code on failure.
 */
status_t process_load_switch(const char* filename, struct process** process);

/**
 * Returns the first available process slot (PID) or an error if all slots are occupied.
 *
 * @return PID index on success, STATUS_ERR(EBUSY) if no slots are free.
 */
status_t process_get_free_slot();

/**
 * Allocates memory for a process and tracks the allocation for cleanup on process termination.
 * 
 * @param process The process to allocate memory for.
 * @param size The size of the memory to allocate.
 * @return Pointer to the allocated memory, or NULL on failure.
 */
void* process_malloc(struct process* process, size_t size);

/**
 * Frees memory allocated for a process and removes it from the process's allocation tracking.
 * 
 * @param process The process that owns the memory allocation.
 * @param ptr The pointer to the memory to free.
 */
void process_free(struct process* process, void* ptr)

#endif // PROCESS_H
