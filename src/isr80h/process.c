#include "isr80h/process.h"
#include "task/task.h"
#include "task/process.h"
#include "status.h"
#include "string/string.h"

void* isr80h_command5_process_load_start(struct interrupt_frame* frame)
{
    void* filename_usr_ptr = task_get_stack_item(task_current(), 0);
    char filename[MAX_PATH];

    status_t res = copy_string_from_task(task_current(), filename, filename_usr_ptr, sizeof(filename));
    
    if (status_is_error(res)) {
        return (void*)res;
    }

    char path[MAX_PATH];
    if (!safe_strcpy(path, sizeof(path), "0:/") ||
        !safe_strcat(path, sizeof(path), filename)) {
        return (void*)STATUS_ERR(ENAMETOOLONG);
    }

    struct process* process = 0;
    res = process_load_switch(path, &process);
    
    if (status_is_error(res)) {
        return (void*)res;
    }

    res = task_switch(process->main_thread);

    task_return(&process->main_thread->registers);

    return process;
}

void* isr80h_command6_invoke_system_command(struct interrupt_frame* frame)
{
    struct command_argument* args = task_virtual_to_physical(task_current(), task_get_stack_item(task_current(), 0));
    
    if (!args || strlen(args->argument) == 0) {
        return (void*)STATUS_ERR(EINVAL);
    }

    struct command_argument* root_command_argument = &args[0];
    const char* program_name = root_command_argument->argument;

    char path[MAX_PATH];
    if (!safe_strcpy(path, sizeof(path), "0:/") ||
        !safe_strcat(path, sizeof(path), program_name)) {
        return (void*)STATUS_ERR(ENAMETOOLONG);
    }

    struct process* process = 0;
    status_t res = process_load_switch(path, &process);

    if (status_is_error(res)) {
        return (void*)res;
    }
    
    res = process_inject_arguments(process, root_command_argument);
    
    if (status_is_error(res)) {
        return (void*)res;
    }

    task_switch(process->main_thread);
    task_return(&process->main_thread->registers);

    return 0;
}

void* isr80h_command7_get_process_arguments(struct interrupt_frame* frame)
{
    struct process* process = task_current()->process;
    struct process_arguments* args = task_virtual_to_physical(task_current(), task_get_stack_item(task_current(), 0));
    process_get_arguments(process, &args->argc, &args->argv);
    return 0;
}

void* isr80h_command8_exit(struct interrupt_frame* frame)
{
    // note there is a int argument for the exit code, but we currently ignore it since we don't have a way to return it to the parent process. We can implement this in the future if needed.
    struct process* process = task_current()->process;
    process_terminate(process);
    task_next();
}
