#include "isr80h/io.h"
#include "console/console.h"
#include "idt/idt.h"
#include "task/task.h"
#include "keyboard/keyboard.h"

void* isr80h_command0_print(struct interrupt_frame* frame)
{
    print("[SYS0]\n");

    void* user_space_msg_buffer = task_get_stack_item(task_current(), 0);
    char buf[4096];
    if (status_is_error(copy_string_from_task(task_current(), buf, user_space_msg_buffer, sizeof(buf) - 1))) {
        return 0;
    }

    buf[sizeof(buf) - 1] = 0;
    print(buf);
    return 0;
}

void* isr80h_command1_getkey(struct interrupt_frame* frame)
{
    return (void*)(uintptr_t)keyboard_pop();
    
}

void* isr80h_command2_putchar(struct interrupt_frame* frame)
{
    char c = (char)(uintptr_t)task_get_stack_item(task_current(), 0);
    print_char(c);
    return 0;
}
