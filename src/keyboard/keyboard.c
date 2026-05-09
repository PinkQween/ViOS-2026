#include "keyboard/keyboard.h"
#include "status.h"
#include "task/process.h"
#include "task/task.h"
#include "keyboard/ps2.h"

static struct keyboard* keyboard_list_head = 0;
static struct keyboard* keyboard_list_tail = 0;

status_t keyboard_insert(struct keyboard* keyboard)
{
    if (!keyboard || !keyboard->init) {
        return STATUS_ERR(EINVAL);
    }

    if (!keyboard_list_head) {
        keyboard_list_head = keyboard;
        keyboard_list_tail = keyboard;
    } else {
        keyboard_list_tail->next = keyboard;
        keyboard_list_tail = keyboard;
    }

    return keyboard->init();
}

status_t keyboard_init()
{
    return keyboard_insert(ps2_init());
}

static int keyboard_get_tail_index(struct process* process)
{
    return process->keyboard.tail % sizeof(process->keyboard.buffer);
}

void keyboard_backspace(struct process* process)
{
    process->keyboard.tail--;
    int real_index = keyboard_get_tail_index(process);
    process->keyboard.buffer[real_index] = 0;
}

void keyboard_push(char c)
{
    struct process* process = process_current();

    if (!process) {
        return;
    }

    if (!process->main_thread) {
        return;
    }

    if (c == 0x00) {
        return;
    }

    int real_index = keyboard_get_tail_index(process);

    process->keyboard.buffer[real_index] = c;
    process->keyboard.tail++;
}

char keyboard_pop()
{
    if (!task_current() || !task_current()->process) {
        return 0;
    }

    struct process* process = task_current()->process;
    int real_index = process->keyboard.head % sizeof(process->keyboard.buffer);
    char c = process->keyboard.buffer[real_index];
    
    if (c == 0) {
        return 0;
    }

    process->keyboard.buffer[real_index] = 0;
    process->keyboard.head++;

    return c;
}

void keyboard_set_caps_lock(struct keyboard* keyboard, bool enabled)
{
    keyboard->caps_lock_enabled = enabled;
}

bool keyboard_get_caps_lock(struct keyboard* keyboard)
{
    return keyboard->caps_lock_enabled;
}