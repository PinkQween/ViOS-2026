#include "isr80h/process.h"
#include "console/console.h"
#include "task/task.h"
#include "task/process.h"
#include "status.h"
#include "string/string.h"
#include "memory/paging/paging.h"
#include "memory/heap/kheap.h"

void* isr80h_command5_process_load_start(struct interrupt_frame* frame)
{
    (void)frame;

    struct task* current = task_current();

    const char* filename_user =
        (const char*)task_get_stack_item(current, 0);

    if (!filename_user)
    {
        return (void*)(intptr_t)STATUS_ERR(EINVAL);
    }

    char filename[MAX_PATH];

    status_t status =
        copy_string_from_task(
            current,
            filename,
            filename_user,
            sizeof(filename)
        );

    if (status_is_error(status))
    {
        return (void*)(intptr_t)status;
    }

    char path[MAX_PATH];

    if (!safe_strcpy(path, sizeof(path), "0:/") ||
        !safe_strcat(path, sizeof(path), filename))
    {
        return (void*)(intptr_t)STATUS_ERR(ENAMETOOLONG);
    }

    struct process* process = NULL;

    status =
        process_load_switch(path, &process);

    if (status_is_error(status))
    {
        return (void*)(intptr_t)status;
    }

    status = task_switch(process->main_thread);

    if (status_is_error(status))
    {
        return (void*)(intptr_t)status;
    }

    // Validate registers before return
    struct registers* regs = &process->main_thread->registers;
    if (regs->rsp < 0x3FB000 || regs->rsp > 0x3FF000)
    {
        print_w_color("[WARN] Invalid RSP before user return!\n", 0x0C);
    }
    if (regs->rip < 0x400000 || regs->rip > 0x500000)
    {
        print_w_color("[WARN] Suspicious RIP before user return!\n", 0x0C);
    }

    task_return(&process->main_thread->registers);

    return process;
}

void* isr80h_command6_invoke_system_command(struct interrupt_frame* frame)
{
    (void)frame;

    struct task* current = task_current();
    void* user_args_virt = task_get_stack_item(current, 0);

    if (!user_args_virt)
    {
        return (void*)(intptr_t)STATUS_ERR(EINVAL);
    }

    struct command_argument* user_node = (struct command_argument*)user_args_virt;

    // Build a kernel-side copy of the argument linked list so we can safely
    // access strings and next pointers while still running in the caller's
    // address space.
    struct command_argument* root_arg = NULL;
    struct command_argument* tail = NULL;

    while (user_node)
    {
        char tmp[4096];
        status_t s = copy_string_from_task(current, tmp, (const char*)user_node, sizeof(tmp) - 1);
        tmp[sizeof(tmp) - 1] = '\0';
        if (status_is_error(s))
        {
            // Cleanup any allocated nodes
            struct command_argument* p = root_arg;
            while (p)
            {
                struct command_argument* n = p->next;
                kfree(p);
                p = n;
            }
            return (void*)(intptr_t)STATUS_ERR(EFAULT);
        }

        struct command_argument* knode = kzalloc(sizeof(struct command_argument));
        if (!knode)
        {
            struct command_argument* p = root_arg;
            while (p)
            {
                struct command_argument* n = p->next;
                kfree(p);
                p = n;
            }
            return (void*)(intptr_t)STATUS_ERR(ENOMEM);
        }

        strncpy(knode->argument, tmp, sizeof(knode->argument));
        knode->next = NULL;

        if (!root_arg)
        {
            root_arg = knode;
            tail = knode;
        }
        else
        {
            tail->next = knode;
            tail = knode;
        }

        void* next_ptr = NULL;
        s = copy_from_task(current, &next_ptr, (void*)((uintptr_t)user_node + sizeof(knode->argument)), sizeof(void*));
        if (status_is_error(s))
        {
            break;
        }

        user_node = (struct command_argument*)next_ptr;
    }

    if (strlen(root_arg->argument) == 0)
    {
        return (void*)(intptr_t)STATUS_ERR(EINVAL);
    }

    char path[MAX_PATH];

    if (!safe_strcpy(path, sizeof(path), "0:/") ||
        !safe_strcat(path, sizeof(path), root_arg->argument))
    {
        return (void*)(intptr_t)STATUS_ERR(ENAMETOOLONG);
    }

    struct process* process = NULL;

    status_t status =
        process_load(path, &process);

    if (status_is_error(status))
    {
        return (void*)(intptr_t)status;
    }

    status =
        process_inject_arguments(
            process,
            root_arg
        );

    if (status_is_error(status))
    {
        process_terminate(process);
        return (void*)(intptr_t)status;
    }

    status = process_switch(process);

    if (status_is_error(status))
    {
        process_terminate(process);
        return (void*)(intptr_t)status;
    }

    // Validate registers before return
    struct registers* regs2 = &process->main_thread->registers;
    if (regs2->rsp < 0x3FB000 || regs2->rsp > 0x3FF000)
    {
        print_w_color("[WARN] Invalid RSP before user return (cmd6)!\n", 0x0C);
    }
    if (regs2->rip < 0x400000 || regs2->rip > 0x500000)
    {
        print_w_color("[WARN] Suspicious RIP before user return (cmd6)!\n", 0x0C);
    }

    task_return(&process->main_thread->registers);

    return NULL;
}

void* isr80h_command7_get_process_arguments(struct interrupt_frame* frame)
{
    (void)frame;

    struct task* current = task_current();
    struct process* process = current->process;

    void* user_args_virt = task_get_stack_item(current, 0);
    
    if (!user_args_virt)
    {
        return (void*)(intptr_t)STATUS_ERR(EINVAL);
    }

    struct process_arguments* user_args =
        (struct process_arguments*)
        task_virtual_to_physical(current, user_args_virt);

    if (!user_args)
    {
        return (void*)(intptr_t)STATUS_ERR(EFAULT);
    }

    user_args->argc = process->arguments.argc;
    user_args->argv = process->arguments.argv;

    return (void*)(intptr_t)STATUS_OK;
}

void* isr80h_command8_exit(struct interrupt_frame* frame)
{
    (void)frame;

    struct process* process =
        task_current()->process;

    process_terminate(process);

    task_next();

    return NULL;
}