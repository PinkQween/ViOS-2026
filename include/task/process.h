#ifndef PROCESS_H
#define PROCESS_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file process.h
 * @brief Kernel process structures and process management API.
 *
 * Handles process loading, virtual memory tracking,
 * task ownership, argument passing, and process cleanup.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "config.h"
#include "status.h"
#include "task/task.h"
#include "memory/paging/paging.h"
#include "loader/elf/elfloader.h"
#include "keyboard/keyboard.h"
#include "fs/file.h"

/**
 * @brief Supported process executable types.
 */
enum
{
    PROCESS_TYPE_ELF = 0,
    PROCESS_TYPE_BINARY = 1
};

typedef uint8_t PROCESS_FILE_TYPE;

/**
 * @brief Represents a tracked process allocation.
 */
struct process_allocation
{
    /**
     * @brief User-space virtual address.
     */
    void* user_ptr;

    /**
     * @brief Kernel/physical allocation backing memory.
     */
    void* kernel_ptr;

    /**
     * @brief Allocation size in bytes.
     */
    size_t size;
};

/**
 * @brief Parsed command-line argument node.
 */
struct command_argument
{
    char argument[4096];
    struct command_argument* next;
};

/**
 * @brief Process argument storage.
 */
struct process_arguments
{
    int argc;
    char** argv;
};

/**
 * @brief Per-process keyboard input buffer.
 */
struct keyboard_buffer
{
    char buffer[KEYBOARD_BUFFER_SIZE];
    int head;
    int tail;
};

struct process_file_handle
{
    int fd;
    char filepath[MAX_PATH];
    char mode[2];
};

/**
 * @brief Represents a loaded user-space process.
 */
struct process
{
    /**
     * @brief Process identifier.
     */
    uint32_t pid;

    /**
     * @brief Executable filename.
     */
    char filename[MAX_PATH];

    /**
     * @brief Process paging descriptor.
     */
    struct paging_desc* paging_desc;

    /**
    * @brief Main thread of execution for this process.
    */
    struct task* main_thread;

    /**
     * @brief Tracked allocations owned by this process.
     */
    struct process_allocation allocations[MAX_PROGRAM_ALLOCATIONS];
    struct vector* file_handles;

    /**
     * @brief Executable type.
     */
    PROCESS_FILE_TYPE filetype;

    /**
     * @brief Executable program data.
     */
    union
    {
        /**
         * @brief Raw binary pointer.
         */
        void* ptr;

        /**
         * @brief ELF file structure.
         */
        struct elf_file* elf;
    };

    /**
     * @brief User-space stack allocation.
     */
    void* stack;

    /**
     * @brief Loaded executable size.
     */
    size_t size;

    /**
     * @brief Keyboard buffer for stdin handling.
     */
    struct keyboard_buffer keyboard;

    /**
     * @brief Process command-line arguments.
     */
    struct process_arguments arguments;
};

/**
 * @brief Currently active process.
 */
extern struct process* current_process;

/**
 * @brief Global process table.
 */
extern struct process* processes[MAX_PROCESSES];

/* -------------------------------------------------------------------------- */
/* Process Management                                                         */
/* -------------------------------------------------------------------------- */

/**
 * @brief Returns the currently running process.
 *
 * @return Current process or NULL.
 */
struct process* process_current(void);

/**
 * @brief Retrieves a process by PID.
 *
 * @param pid Process identifier.
 *
 * @return Process pointer or NULL.
 */
struct process* process_get(int pid);

/**
 * @brief Returns a free process slot.
 *
 * @return Slot index or STATUS_ERR(EBUSY).
 */
status_t process_get_free_slot(void);

/**
 * @brief Loads a process executable.
 *
 * @param filename Executable path.
 * @param process_out Loaded process output.
 *
 * @return STATUS_OK on success.
 */
status_t process_load(
    const char* filename,
    struct process** process_out
);

/**
 * @brief Loads a process into a specific slot.
 *
 * @param filename Executable path.
 * @param process_out Loaded process output.
 * @param process_slot Target process slot.
 *
 * @return STATUS_OK on success.
 */
status_t process_load_for_slot(
    const char* filename,
    struct process** process_out,
    int process_slot
);

/**
 * @brief Loads and switches to a process.
 *
 * @param filename Executable path.
 * @param process_out Loaded process output.
 *
 * @return STATUS_OK on success.
 */
status_t process_load_switch(
    const char* filename,
    struct process** process_out
);

/**
 * @brief Switches execution to a process.
 *
 * @param process Target process.
 *
 * @return STATUS_OK on success.
 */
status_t process_switch(struct process* process);

/**
 * @brief Terminates a process and frees resources.
 *
 * @param process Process to terminate.
 *
 * @return STATUS_OK on success.
 */
status_t process_terminate(struct process* process);

/**
 * @brief Switches to any available process.
 */
void process_switch_to_any(void);

/* -------------------------------------------------------------------------- */
/* Memory Management                                                          */
/* -------------------------------------------------------------------------- */

/**
 * @brief Maps process executable memory.
 *
 * @param process Target process.
 *
 * @return STATUS_OK on success.
 */
status_t process_map_memory(struct process* process);

/**
 * @brief Converts a process virtual address to physical.
 *
 * @param process Target process.
 * @param virt_addr Virtual address.
 *
 * @return Physical address or NULL.
 */
void* process_virtual_address_to_physical(
    struct process* process,
    void* virt_addr
);

/**
 * @brief Allocates memory owned by a process.
 *
 * @param process Target process.
 * @param size Allocation size.
 *
 * @return User virtual pointer or NULL.
 */
void* process_malloc(
    struct process* process,
    size_t size
);

/**
 * @brief Returns the kernel pointer for a process allocation.
 *
 * @param process Target process.
 * @param user_ptr User virtual pointer.
 *
 * @return Kernel virtual pointer or NULL.
 */
void* process_malloc_get_kernel_ptr(struct process* process, void* user_ptr);

/**
 * @brief Frees process-owned memory.
 *
 * @param process Target process.
 * @param ptr User virtual pointer.
 */
void process_free(
    struct process* process,
    void* ptr
);

/**
 * @brief Frees all tracked allocations.
 *
 * @param process Target process.
 */
void process_terminate_allocations(
    struct process* process
);

/* -------------------------------------------------------------------------- */
/* Program Data                                                               */
/* -------------------------------------------------------------------------- */

/**
 * @brief Frees loaded binary data.
 *
 * @param process Target process.
 */
void process_free_binary_data(
    struct process* process
);

/**
 * @brief Frees loaded ELF data.
 *
 * @param process Target process.
 */
void process_free_elf_data(
    struct process* process
);

/**
 * @brief Frees executable program data.
 *
 * @param process Target process.
 *
 * @return STATUS_OK on success.
 */
status_t process_free_program_data(
    struct process* process
);

/* -------------------------------------------------------------------------- */
/* Arguments                                                                  */
/* -------------------------------------------------------------------------- */

/**
 * @brief Retrieves process argc/argv.
 *
 * @param process Target process.
 * @param argc Output argc.
 * @param argv Output argv.
 */
void process_get_arguments(
    struct process* process,
    int* argc,
    char*** argv
);

/**
 * @brief Counts parsed command arguments.
 *
 * @param root_arg Argument list head.
 *
 * @return Number of arguments.
 */
int process_count_command_arguments(
    struct command_argument* root_arg
);

/**
 * @brief Injects arguments into process memory.
 *
 * @param process Target process.
 * @param root_arg Argument list head.
 *
 * @return STATUS_OK on success.
 */
status_t process_inject_arguments(
    struct process* process,
    struct command_argument* root_arg
);

/**
 * 
 */
status_t process_fopen(struct process* process, const char* filepath, const char* mode);

/**
 * 
 */
struct process_file_handle* process_get_file_handle(struct process* process, int fd);

#endif /* PROCESS_H */