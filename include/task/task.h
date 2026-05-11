#ifndef TASK_H
#define TASK_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file task.h
 * @brief Kernel task scheduling and CPU context management.
 *
 * Provides task creation, task switching, register state
 * management, paging helpers, and task memory utilities.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include <stdint.h>

#include "config.h"
#include "status.h"
#include "idt/idt.h"
#include "memory/paging/paging.h"

struct process;

/**
 * @brief Saved CPU register state for a task.
 */
struct registers
{
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rbp;
    uint64_t rbx;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rax;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
};

/**
 * @brief Represents a schedulable kernel task.
 */
struct task
{
    /**
     * @brief Saved CPU registers for this task.
     */
    struct registers registers;

    /**
     * @brief Next task in scheduler linked list.
     */
    struct task* next;

    /**
     * @brief Previous task in scheduler linked list.
     */
    struct task* prev;

    /**
     * @brief Owning process.
     */
    struct process* process;
};

/**
 * @brief Currently running task.
 */
extern struct task* current_task;

/**
 * @brief Head of the scheduler task list.
 */
extern struct task* task_head;

/**
 * @brief Tail of the scheduler task list.
 */
extern struct task* task_tail;

/**
 * @brief Returns the currently running task.
 *
 * @return Current task pointer.
 */
struct task* task_current(void);

/**
 * @brief Creates a new task for a process.
 *
 * @param process Owning process.
 *
 * @return Newly created task or NULL on failure.
 */
struct task* task_new(struct process* process);

/**
 * @brief Frees a task and removes it from the scheduler list.
 *
 * @param task Task to free.
 */
void task_free(struct task* task);

/**
 * @brief Initializes a task structure.
 *
 * @param task Task to initialize.
 * @param process Owning process.
 *
 * @return STATUS_OK on success.
 */
status_t task_init(struct task* task, struct process* process);

/**
 * @brief Switches execution to another task.
 *
 * Updates the current task pointer and switches
 * to the task's paging descriptor.
 *
 * @param task Task to switch to.
 *
 * @return STATUS_OK on success.
 */
status_t task_switch(struct task* task);

/**
 * @brief Switches to the next scheduled task.
 */
void task_next(void);

/**
 * @brief Switches to the current task paging context.
 */
status_t task_page(void);

/**
 * @brief Runs the first/root task.
 */
void task_run_root_task(void);

/**
 * @brief Saves interrupt frame state into a task.
 *
 * @param task Target task.
 * @param frame Interrupt frame.
 */
void task_save_state(
    struct task* task,
    struct interrupt_frame* frame
);

/**
 * @brief Saves the current task state.
 *
 * @param frame Interrupt frame.
 */
void task_current_save_state(
    struct interrupt_frame* frame
);

/**
 * @brief Restores CPU registers from a task state.
 *
 * Implemented in assembly.
 *
 * @param registers Register state.
 */
void restore_general_purpose_registers(
    struct registers* registers
);

/**
 * @brief Returns to user mode using saved registers.
 *
 * Implemented in assembly.
 *
 * @param registers Register state.
 */
void task_return(struct registers* registers);

/**
 * @brief Prepares user-mode segment registers.
 *
 * Implemented in assembly.
 */
void user_registers(void);

/**
 * @brief Returns the paging descriptor for a task.
 *
 * @param task Target task.
 *
 * @return Paging descriptor.
 */
struct paging_desc* task_get_paging_descriptor(
    struct task* task
);

/**
 * @brief Returns the current task paging descriptor.
 *
 * @return Current paging descriptor.
 */
struct paging_desc* task_current_paging_descriptor(void);

/**
 * @brief Copies a string from task memory.
 *
 * @param task Source task.
 * @param dest Destination kernel buffer.
 * @param src Source user pointer.
 * @param max_len Maximum string length.
 *
 * @return STATUS_OK on success.
 */
status_t copy_string_from_task(
    struct task* task,
    void* dest,
    const char* src,
    int max_len
);

/**
 * @brief Copies arbitrary bytes from a task's virtual address into kernel memory.
 */
status_t copy_from_task(
    struct task* task,
    void* dest,
    const void* src,
    size_t size
);

/**
 * @brief Returns a stack item from a task.
 *
 * @param task Target task.
 * @param index Stack index.
 *
 * @return Pointer-sized stack value.
 */
void* task_get_stack_item(
    struct task* task,
    int index
);

/**
 * @brief Converts a task virtual address to a physical address.
 *
 * @param task Target task.
 * @param virtual_address Virtual address.
 *
 * @return Physical address or NULL.
 */
void* task_virtual_to_physical(
    struct task* task,
    void* virtual_address
);

#endif /* TASK_H */