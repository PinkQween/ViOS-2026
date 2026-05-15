/**
 * Copyright (c) 2026 Hanna Skairipa.
 *
 * @file task.c
 * @brief Task scheduler and task management implementation.
 */

#include "task/task.h"

#include "console/console.h"
#include "kernel.h"
#include "loader/elf/elfloader.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "task/process.h"
#include "string/string.h"

struct task* current_task = NULL;

struct task* task_head = NULL;
struct task* task_tail = NULL;

status_t task_init(struct task* task, struct process* process);
static struct task* task_get_next_task(void);
static void task_list_remove(struct task* task);
static status_t task_page_task(struct task* task);

struct task* task_current(void)
{
    return current_task;
}

struct task* task_new(struct process* process)
{
    struct task* task = kzalloc(sizeof(struct task));

    if (!task)
    {
        return NULL;
    }

    status_t res = task_init(task, process);

    if (status_is_error(res))
    {
        kfree(task);
        return NULL;
    }

    if (!task_head)
    {
        task_head = task;
        task_tail = task;
        current_task = task;

        return task;
    }

    task_tail->next = task;
    task->prev = task_tail;
    task_tail = task;

    return task;
}

static struct task* task_get_next_task(void)
{
    if (!current_task)
    {
        return NULL;
    }

    if (!current_task->next)
    {
        return task_head;
    }

    return current_task->next;
}

static void task_list_remove(struct task* task)
{
    if (!task)
    {
        return;
    }

    if (task->prev)
    {
        task->prev->next = task->next;
    }

    if (task->next)
    {
        task->next->prev = task->prev;
    }

    if (task == task_head)
    {
        task_head = task->next;
    }

    if (task == task_tail)
    {
        task_tail = task->prev;
    }

    if (task == current_task)
    {
        current_task = task_get_next_task();
    }
}

void task_free(struct task* task)
{
    if (!task)
    {
        return;
    }

    task_list_remove(task);

    kfree(task);
}

void task_next(void)
{
    struct task* next_task = task_get_next_task();

    if (!next_task)
    {
        panic("No tasks available!\n");
    }

    task_switch(next_task);

    task_return(&next_task->registers);
}

status_t task_switch(struct task* task)
{
    if (!task || !task->process)
    {
        return STATUS_ERR(EINVAL);
    }

    current_task = task;
    current_process = task->process;

    paging_switch(task->process->paging_desc);

    return STATUS_OK;
}

struct paging_desc* task_get_paging_descriptor(struct task* task)
{
    if (!task || !task->process)
    {
        return NULL;
    }

    return task->process->paging_desc;
}

struct paging_desc* task_current_paging_descriptor(void)
{
    if (!current_task)
    {
        return NULL;
    }

    return task_get_paging_descriptor(current_task);
}

void task_save_state(struct task* task, struct interrupt_frame* frame)
{
    if (!task || !frame)
    {
        return;
    }

    task->registers.rip = frame->rip;
    task->registers.cs = frame->cs;
    task->registers.rflags = frame->rflags;
    task->registers.rsp = frame->user_rsp;
    task->registers.ss = frame->ss;

    task->registers.rax = frame->rax;
    task->registers.rbx = frame->rbx;
    task->registers.rcx = frame->rcx;
    task->registers.rdx = frame->rdx;

    task->registers.rbp = frame->rbp;
    task->registers.rsi = frame->rsi;
    task->registers.rdi = frame->rdi;
}

void task_current_save_state(struct interrupt_frame* frame)
{
    if (!current_task)
    {
        panic("No current task!\n");
    }

    task_save_state(current_task, frame);
}

status_t task_page(void)
{
    if (!current_task)
    {
        return STATUS_ERR(EINVAL);
    }

    user_registers();

    paging_switch(current_task->process->paging_desc);

    return STATUS_OK;
}

static status_t task_page_task(struct task* task)
{
    if (!task)
    {
        return STATUS_ERR(EINVAL);
    }

    user_registers();

    paging_switch(task->process->paging_desc);

    return STATUS_OK;
}

void task_run_root_task(void)
{
    if (!task_head)
    {
        panic("No root task!\n");
    }

    status_t res = task_switch(task_head);

    if (status_is_error(res))
    {
        panic_status("Failed to switch task", res);
    }

    task_return(&task_head->registers);
}

status_t task_init(struct task* task, struct process* process)
{
    memset(task, 0, sizeof(struct task));

    task->registers.rip = PROGRAM_VIRTUAL_ADDRESS;

    if (process && process->filetype == PROCESS_TYPE_ELF)
    {
        task->registers.rip =
            (uint64_t)elf_entry_point(process->elf);
    }

    task->registers.ss = USER_DATA_SEGMENT;
    task->registers.cs = USER_CODE_SEGMENT;
    task->registers.rflags = 0x202;
    task->registers.rsp = PROGRAM_VIRTUAL_STACK_ADDRESS_START;

    task->process = process;

    return STATUS_OK;
}

status_t copy_string_from_task(struct task* task,
                               void* dest,
                               const char* src,
                               int max_len)
{
    if (!task || !dest || !src)
    {
        return STATUS_ERR(EINVAL);
    }

    if (max_len <= 0 || max_len >= PAGING_PAGE_SIZE)
    {
        return STATUS_ERR(EINVAL);
    }

    char* tmp = kzalloc((size_t)max_len);

    if (!tmp)
    {
        return STATUS_ERR(ENOMEM);
    }

    task_page_task(task);

    strncpy(tmp, src, (size_t)max_len);

    kernel_page();

    strncpy(dest, tmp, (size_t)max_len);

    kfree(tmp);

    return STATUS_OK;
}

status_t copy_from_task(struct task* task,
                       void* dest,
                       const void* src,
                       size_t size)
{
    if (!task || !dest || !src)
    {
        return STATUS_ERR(EINVAL);
    }

    if (size == 0 || size >= PAGING_PAGE_SIZE)
    {
        return STATUS_ERR(EINVAL);
    }

    void* tmp = kzalloc(size);

    if (!tmp)
    {
        return STATUS_ERR(ENOMEM);
    }

    task_page_task(task);

    memcpy(tmp, src, size);

    kernel_page();

    memcpy(dest, tmp, size);

    kfree(tmp);

    return STATUS_OK;
}

void* task_get_stack_item(struct task* task, int index)
{
    if (!task)
    {
        return NULL;
    }

    uint64_t* rsp =
        (uint64_t*)task->registers.rsp;

    void* item = NULL;

    task_page_task(task);

    item = (void*)rsp[index];

    kernel_page();

    return item;
}

void* task_virtual_to_physical(struct task* task,
                               void* virtual_address)
{
    if (!task || !task->process)
    {
        return NULL;
    }

    return paging_get_physical_address(
        task->process->paging_desc,
        virtual_address
    );
}
