#include "isr80h/heap.h"
#include "task/task.h"

void* isr80h_command3_malloc(struct interrupt_frame* frame)
{
    size_t size = (size_t)task_get_stack_item(task_current(), 0);
    return process_malloc(task_current()->process, size);
}

void* isr80h_command4_free(struct interrupt_frame* frame)
{
    void* ptr = (void*)task_get_stack_item(task_current(), 0);
    process_free(task_current()->process, ptr);
    return 0;
}