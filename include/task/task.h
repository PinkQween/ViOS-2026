#ifndef TASK_H
#define TASK_H

#include "config.h"
#include "memory/paging/paging.h"
#include "task/process.h"
#include "status.h"
#include "idt/idt.h"

struct registers
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;

    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
};

struct task
{
    struct paging_4gb_chunk *page_directory;

    struct registers registers;

    struct task* next;

    struct task* prev;

    struct process* process;
};

/**
 * Returns the currently running task.
 * 
 * @return The currently running task.
 */
struct task* task_current();

/**
 * Switches to the next task in the task list, saving the state of the current task and restoring the state of the next task. This function is called by the scheduler to perform a context switch between tasks.
 */
void task_next();

/**
 * Frees a task and removes it from the task list.
 * 
 * @param task The task to free.
 */
void task_free(struct task* task);

/**
 * Initializes a task.
 * 
 * @param task The task to initialize.
 * @return STATUS_OK on success, or an error code on failure.
 */
status_t task_init(struct task* task, struct process* process);

/**
 * Creates a new task and adds it to the task list.
 * 
 * @param process The process to associate with the new task.
 * @return The newly created task, or 0 on failure.
 */
struct task* task_new(struct process* process);

/**
 * 
 * Restores general-purpose registers, segment registers, instruction pointer, and flags from the task's saved state and switches to the task's page directory.
 * 
 * @param registers The registers to restore.
 */
void restore_general_purpose_registers_and_switch(struct registers* registers);

/**
 * Returns from the current task and switches to the next task.
 * 
 * @param registers The registers to use for the switch.
 */
void task_return(struct registers* registers);

/**
 * Sets up the user-mode registers for a new task.
 * 
 */
void user_registers();

/**
 * Switches to the specified task.
 * 
 * @param task The task to switch to.
 * @return STATUS_OK on success, or an error code on failure.
 */
status_t task_switch(struct task* task);

/**
 * Switches to the next task in the task list.
 * 
 */
void task_page();

/**
 * Runs the root task, which is the first task created by the kernel and is responsible for running the initial user-space process. This function should be called after all necessary initialization is complete and the kernel is ready to start executing user-space code.
 */
void task_run_root_task();

/**
 * Loads a user-space process from the specified filename and creates a new task for it.
 * 
 * @param filename The filename of the process to load.
 * @param process A pointer to a process pointer that will be set to the newly created process structure on success.
 * @return STATUS_OK on success, or an error code on failure.
 */
status_t process_load(const char* filename, struct process** process);

/**
 * Saves the state of the currently running task into its task structure. This function should be called during an interrupt or system call to capture the current state of the task before switching to another task or returning to user space.
 * 
 * @param frame The interrupt frame containing the current state of the CPU registers and other information at the time of the interrupt or system call.
 */
void task_current_save_state(struct interrupt_frame* frame);

/**
 * Copies string from task memory and updates the task's registers to point to the string. This function is used to pass string arguments from user space to kernel space when switching to a new task.
 * 
 * @param task The task to copy the string from.
 * @param dest The destination in the task's memory.
 * @param src The source string.
 * @param max_len The maximum length of the string to copy.
 * @return STATUS_OK on success, or an error code on failure.
 */
status_t copy_string_from_task(struct task* task, void* dest, const char* src, int max_len);   

/**
 * Gets an item from the task's stack.
 * 
 * @param task The task to get the stack item from.
 * @param index The index of the item to get.
 * @return A pointer to the requested stack item.
 */
void* task_get_stack_item(struct task* task, int index);

/**
 * Translates a virtual address in the task's address space to a physical address. This function is used to access memory in the task's address space from the kernel.
 * 
 * @param task The task whose address space to translate.
 * @param virtual_address The virtual address to translate.
 * @return The physical address corresponding to the virtual address, or NULL if the virtual address is
 */
void* task_virtual_to_physical(struct task* task, void* virtual_address);

#endif // TASK_H