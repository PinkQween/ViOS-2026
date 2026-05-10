#include "isr80h/process.h"
#include "task/task.h"
#include "task/process.h"
#include "status.h"
#include "string/string.h"
#include "memory/paging/paging.h"

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

    task_return(&process->main_thread->registers);

    return process;
}

void* isr80h_command6_invoke_system_command(struct interrupt_frame* frame)
{
    (void)frame;

    struct task* current = task_current();

    struct command_argument* args =
        (struct command_argument*)
        task_virtual_to_physical(
            current,
            task_get_stack_item(current, 0)
        );

    if (!args)
    {
        return (void*)(intptr_t)STATUS_ERR(EINVAL);
    }

    struct command_argument* root_arg = &args[0];

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
        process_load_switch(path, &process);

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

    status = task_switch(process->main_thread);

    if (status_is_error(status))
    {
        process_terminate(process);
        return (void*)(intptr_t)status;
    }

    task_return(&process->main_thread->registers);

    return NULL;
}

void* isr80h_command7_get_process_arguments(struct interrupt_frame* frame)
{
    (void)frame;

    struct task* current = task_current();

    struct process* process = current->process;

    struct process_arguments* user_args =
        (struct process_arguments*)
        task_virtual_to_physical(
            current,
            task_get_stack_item(current, 0)
        );

    if (!user_args)
    {
        return (void*)(intptr_t)STATUS_ERR(EINVAL);
    }

    user_args->argc = process->arguments.argc;
    user_args->argv = process->arguments.argv;

    return NULL;
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