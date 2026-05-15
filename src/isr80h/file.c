#include "isr80h/file.h"
#include "task/task.h"
#include "task/process.h"
#include "status.h"

void* isr80h_command9_fopen(struct interrupt_frame* frame)
{
    void* filename_address = process_virtual_address_to_physical(task_current()->process, task_get_stack_item(task_current(), 0));

    if (!filename_address)
    {
        return (void*)STATUS_ERR(ENOMEM);
    }

    void* mode_address = process_virtual_address_to_physical(task_current()->process, task_get_stack_item(task_current(), 1));

    if (!mode_address)
    {
        return (void*)STATUS_ERR(ENOMEM);
    }

    status_t fd = process_fopen(task_current()->process, filename_address, mode_address);

    return (void*)fd;
}