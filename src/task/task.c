#include "task/task.h"
#include "console/console.h"
#include "memory/heap/kheap.h"
#include "string/string.h"
#include "memory/paging/paging.h"
#include "memory/memory.h"
#include "kernel.h"

struct task* current_task = 0;

struct task* task_tail = 0;
struct task* task_head = 0;

struct task* task_current()
{
    return current_task;
}

struct task* task_get_next(struct task* task)
{
    if (task->next) {
        return task->next;
    } else {
        return task_head;
    }
}

static void task_list_remove(struct task* task)
{
    if (task->prev) {
        task->prev->next = task->next;
    } else {
        task_head = task->next;
    }

    if (task->next) {
        task->next->prev = task->prev;
    } else {
        task_tail = task->prev;
    }

    if (current_task == task) {
        current_task = task_get_next(current_task);
    }
}

void task_free(struct task* task)
{
    paging_free_4gb(task->page_directory);
    task_list_remove(task);
    kfree(task);
}


status_t task_init(struct task* task, struct process* process)
{
    memset(task, 0, sizeof(struct task));

    task->page_directory = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_ACCESSIBLE_FROM_ALL);

    if (!task->page_directory) {
        return STATUS_ERR(ENOMEM);
    }
    
    task->registers.ip = PROGRAM_VIRTUAL_ADDRESS;
    task->registers.ss = USER_DATA_SEGMENT;
    task->registers.cs = USER_CODE_SEGMENT;
    task->registers.flags = 0x202;
    task->registers.esp = PROGRAM_VIRTUAL_STACK_ADDRESS_START;

    task->process = process;

    return STATUS_OK;
}

status_t task_switch(struct task* task)
{
    if (!task) {
        return STATUS_ERR(EINVAL);
    }

    current_task = task;

    paging_switch(task->page_directory);

    return STATUS_OK;
}

struct task* task_new(struct process* process)
{
    struct task* task = kzalloc(sizeof(struct task));

    if (!task) {
        return 0;
    }

    status_t res = task_init(task, process);

    if (status_is_error(res)) {
        paging_free_4gb(task->page_directory);
        kfree(task);
        return 0;
    }

    if (!task_head) {
        task_head = task;
        task_tail = task;
        current_task = task;
    } else {
        task_tail->next = task;
        task->prev = task_tail;
        task_tail = task;
    }

    return task;
}

void task_page()
{
    user_registers();
    if (!current_task) {
        return;
    }
    paging_switch(current_task->page_directory);
}

void task_run_root_task()
{
    if (!current_task) {
        panic("No root task to run!");    
    }

    status_t res = task_switch(task_head);
    if (status_is_error(res)) {
        panic_status("Failed to switch to root task", res);
    }

    terminal_clear_color_and_reset_cursor(choose_colour(WHITE, BLACK));

    task_return(&task_head->registers);
}

void task_save_state(struct task* task, struct interrupt_frame* frame)
{
    task->registers.ip = frame->ip;
    task->registers.cs = frame->cs;
    task->registers.flags = frame->flags;
    task->registers.esp = frame->esp;
    task->registers.ss = frame->ss;
    task->registers.eax = frame->eax;
    task->registers.ecx = frame->ecx;
    task->registers.edx = frame->edx;
    task->registers.ebx = frame->ebx;
    task->registers.ebp = frame->ebp;
    task->registers.esi = frame->esi;
    task->registers.edi = frame->edi;
}

void task_current_save_state(struct interrupt_frame* frame)
{
    if (!task_current()) {
        panic("No current task to save state for!");
    }

    struct task* task = task_current();
    task_save_state(task, frame);
}

status_t copy_string_from_task(struct task* task, void* dest, const char* src, int max_len)
{
    if (!task || !task->page_directory || !dest || !src || max_len <= 0 || max_len >= PAGING_PAGE_SIZE_BYTES) {
        return STATUS_ERR(EINVAL);
    }

    char* tmp = kzalloc(max_len);
    
    if (!tmp) {
        return STATUS_ERR(ENOMEM);
    }

    uint32_t* task_directory = task->page_directory->directory_entry;
    uint32_t old_entry = paging_get(task_directory, tmp);

    status_t res = paging_map(task->page_directory, tmp, tmp, PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESSIBLE_FROM_ALL);
    if (status_is_error(res)) {
        kfree(tmp);
        return res;
    }

    paging_switch(task->page_directory);

    strncpy(tmp, src, max_len);

    kernel_page();

    res = paging_set(task->page_directory, tmp, old_entry);

    if (status_is_error(res)) {
        kfree(tmp);
        return res;
    }

    strncpy(dest, tmp, max_len);

    kfree(tmp);
    
    return STATUS_OK;
}

void task_page_task(struct task* task)
{
    user_registers();

    paging_switch(task->page_directory);
}

void* task_get_stack_item(struct task* task, int index)
{
    uint32_t* esp = (uint32_t*)task->registers.esp;
    void* item = 0;
    
    task_page_task(task);
    item = (void*)esp[index];

    kernel_page();

    return item;
}

void* task_virtual_to_physical(struct task* task, void* virtual_address)
{
    if (!task || !task->page_directory || !virtual_address) {
        return NULL;
    }
    
    return paging_get_physical_address(task->page_directory->directory_entry, virtual_address);
}

void task_next()
{
    struct task* next_task = task_get_next(current_task);

    if (!next_task) {
        task_run_root_task();
        return;
    }

    task_switch(next_task);
    task_return(&next_task->registers);
}