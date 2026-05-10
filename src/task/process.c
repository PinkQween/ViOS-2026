#include "task/process.h"
#include "task/task.h"
#include "config.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "fs/file.h"
#include "loader/elf/elfloader.h"
#include "loader/elf/elf.h"
#include "string/string.h"
#include "console/console.h"

struct process* current_process = NULL;
struct process* processes[MAX_PROCESSES] = {0};

static void process_init(struct process* process)
{
    memset(process, 0, sizeof(struct process));
}

struct process* process_current()
{
    return current_process;
}

struct process* process_get(int pid)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
        return NULL;

    return processes[pid];
}

static status_t process_load_binary(const char* filename, struct process* process)
{
    status_t fd = fopen(filename, "rb");
    if (status_is_error(fd))
        return fd;

    struct file_stat stat;
    status_t stat_result = fstat(fd, &stat);
    if (status_is_error(stat_result)) {
        fclose(fd);
        return stat_result;
    }

    void* program_data = kzalloc(stat.size);
    if (!program_data) {
        fclose(fd);
        return STATUS_ERR(ENOMEM);
    }

    status_t read_result = fread(program_data, 1, stat.size, fd);
    if (status_is_error(read_result) || read_result != stat.size) {
        kfree(program_data);
        fclose(fd);
        return STATUS_ERR(EIO);
    }

    process->ptr = program_data;
    process->size = stat.size;
    process->filetype = PROCESS_TYPE_BINARY;

    fclose(fd);
    return STATUS_OK;
}

static status_t process_load_elf(const char* filename, struct process* process)
{
    struct elf_file* elf = NULL;
    status_t res = elf_load(filename, &elf);
    if (status_is_error(res))
        return res;

    res = elf_load_needed_libraries(elf);
    if (status_is_error(res)) {
        elf_close(elf);
        return res;
    }

    res = elf_apply_relocations(elf);
    if (status_is_error(res)) {
        elf_close(elf);
        return res;
    }

    process->elf = elf;
    process->filetype = PROCESS_TYPE_ELF;
    return STATUS_OK;
}

static status_t process_load_data(const char* filename, struct process* process)
{
    status_t res = process_load_elf(filename, process);
    if (status_is_ok(res))
        return res;

    return process_load_binary(filename, process);
}

static status_t process_map_binary(struct process* process)
{
    return paging_map_to(
        process->main_thread->page_directory,
        (void*)PROGRAM_VIRTUAL_ADDRESS,
        process->ptr,
        paging_align_address((char*)process->ptr + process->size),
        PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESSIBLE_FROM_ALL
    );
}

static status_t process_map_elf_file(struct process* process, struct elf_file* elf_file)
{
    for (int i = 0; i < elf_file->segment_count; i++) {
        struct elf_segment* seg = &elf_file->segments[i];
        
        int flags = PAGING_IS_PRESENT | PAGING_ACCESSIBLE_FROM_ALL;
        if (seg->flags & PF_W) flags |= PAGING_IS_WRITEABLE;
        
        status_t res = paging_map_to(
            process->main_thread->page_directory,
            paging_align_to_lower_page(seg->vaddr),
            seg->paddr,
            paging_align_address((char*)seg->paddr + seg->memsz),
            flags
        );

        if (status_is_error(res))
            return res;

        // Zero BSS if memsz > filesz
        if (seg->memsz > seg->filesz) {
            memset((char*)seg->paddr + seg->filesz, 0, seg->memsz - seg->filesz);
        }
    }

    for (int i = 0; i < elf_file->needed_count; i++) {
        status_t res = process_map_elf_file(process, elf_file->needed_libraries[i]);
        if (status_is_error(res))
            return res;
    }

    return STATUS_OK;
}

static status_t process_map_elf(struct process* process)
{
    if (!process || !process->elf)
        return STATUS_ERR(EINVAL);

    return process_map_elf_file(process, process->elf);
}

status_t process_map_memory(struct process* process)
{
    if (!process)
        return STATUS_ERR(EINVAL);

    status_t res = STATUS_OK;

    switch (process->filetype) {
        case PROCESS_TYPE_ELF:
            res = process_map_elf(process);
            break;
        case PROCESS_TYPE_BINARY:
            res = process_map_binary(process);
            break;
        default:
            return STATUS_ERR(EINVAL);
    }

    if (status_is_error(res))
        return res;

    return paging_map_to(
        process->main_thread->page_directory,
        (void*)PROGRAM_VIRTUAL_STACK_ADDRESS_END,
        process->stack,
        paging_align_address((char*)process->stack + USER_PROGRAM_STACK_SIZE),
        PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESSIBLE_FROM_ALL
    );
}

status_t process_get_free_slot()
{
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (!processes[i])
            return i;
    }
    return STATUS_ERR(EBUSY);
}

status_t process_load_for_slot(const char* filename, struct process** process_out, int process_slot)
{
    if (!filename || !process_out)
        return STATUS_ERR(EINVAL);

    status_t res = STATUS_OK;
    struct process* new_process = kzalloc(sizeof(struct process));
    if (!new_process)
        return STATUS_ERR(ENOMEM);

    process_init(new_process);

    res = process_load_data(filename, new_process);
    if (status_is_error(res))
        goto fail;

    new_process->stack = kzalloc(USER_PROGRAM_STACK_SIZE);
    if (!new_process->stack) {
        res = STATUS_ERR(ENOMEM);
        goto fail;
    }

    strncpy(new_process->filename, filename, sizeof(new_process->filename));
    new_process->pid = process_slot;

    new_process->main_thread = task_new(new_process);
    if (!new_process->main_thread) {
        res = STATUS_ERR(ENOMEM);
        goto fail;
    }

    if (new_process->filetype == PROCESS_TYPE_ELF) {
        new_process->main_thread->registers.rip = (uint64_t)elf_entry_point(new_process->elf);
    }

    res = process_map_memory(new_process);
    if (status_is_error(res))
        goto fail;

    *process_out = new_process;
    processes[process_slot] = new_process;
    return STATUS_OK;

fail:
    if (new_process->main_thread)
        task_free(new_process->main_thread);

    if (new_process->stack)
        kfree(new_process->stack);

    if (new_process->ptr)
        kfree(new_process->ptr);

    if (new_process->elf)
        elf_close(new_process->elf);

    kfree(new_process);
    return res;
}

status_t process_load(const char* filename, struct process** process_out)
{
    status_t slot = process_get_free_slot();
    if (status_is_error(slot))
        return STATUS_ERR(EBUSY);

    return process_load_for_slot(filename, process_out, slot);
}

static status_t process_find_free_allocation_index(struct process* process)
{
    for (int i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++) {
        if (!process->allocations[i].ptr)
            return i;
    }
    return STATUS_ERR(ENOMEM);
}

void* process_malloc(struct process* process, size_t size)
{
    void* ptr = kzalloc(size);

    if (!ptr)
        return NULL;

    int index = process_find_free_allocation_index(process);

    if (status_is_error(index))
        return NULL;

    if (status_is_error(
        paging_map_to(
            process->main_thread->page_directory,
            ptr,
            ptr,
            paging_align_address((char*)ptr + size),
            PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESSIBLE_FROM_ALL
        )
    )) {
        kfree(ptr);
        return NULL;
    }
    
    process->allocations[index].ptr = ptr;
    process->allocations[index].size = size;

    return ptr;
}

static bool process_is_process_allocation(struct process* process, void* ptr)
{
    for (int i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++) {
        if (process->allocations[i].ptr == ptr)
            return true;
    }
    return false;
}

static void process_allocation_unjoin(struct process* process, void* ptr)
{
    for (int i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++) {
        if (process->allocations[i].ptr == ptr) {
            process->allocations[i].ptr = NULL;
            process->allocations[i].size = 0;
        }
    }
}

void process_terminate_allocations(struct process* process)
{
    for (int i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++) {
        process_free(process, process->allocations[i].ptr);
    }
}

void process_free_binary_data(struct process* process)
{
    if (process->ptr) {
        kfree(process->ptr);
        process->ptr = NULL;
    }
}

void process_free_elf_data(struct process* process)
{
    elf_close(process->elf);
}

status_t process_free_program_data(struct process* process)
{
    switch (process->filetype)
    {
    case PROCESS_TYPE_ELF:
        process_free_elf_data(process);
        break;
    case PROCESS_TYPE_BINARY:
        process_free_binary_data(process);
    default:
        return STATUS_ERR(EINVAL);
    }

    return 0;
}

void process_switch_to_any()
{
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (processes[i]) {
            process_switch(processes[i]);
            return;
        }
    }

    struct process* process = NULL;
    status_t res = process_load_switch(ROOT_PROCESS_PATH, &process);
    if (status_is_error(res)) {
        panic_status("Failed to reload root process", res);
    }
}

static void process_unlink(struct process* process)
{
    processes[process->pid] = NULL;

    if (current_process == process)
        process_switch_to_any();
}

status_t process_terminate(struct process* process)
{
    process_terminate_allocations(process);

    status_t res = process_free_program_data(process);

    if (status_is_error(res))
        return res;

    kfree(process->stack);
    task_free(process->main_thread);

    process_unlink(process);

    kfree(process);

    return STATUS_OK;
}

void process_get_arguments(struct process* process, int *argc, char*** argv)
{
    if (!process || !argv || !argc)
        return;

    *argc = process->arguments.argc;
    *argv = process->arguments.argv;
}

int process_count_command_arguments(struct command_argument* root_arg)
{
    int count = 0;
    struct command_argument* current = root_arg;
    
    while (current) {
        count++;
        current = current->next;
    }

    return count;
}

status_t process_inject_arguments(struct process* process, struct command_argument* root_arg)
{
    status_t res = STATUS_OK;
    
    struct command_argument* current = root_arg;

    int index = 0;
    int argc = process_count_command_arguments(root_arg);

    if (argc == 0)
        return STATUS_ERR(EINVAL);

    char** argv = process_malloc(process, sizeof(char*) * argc);

    if (!argv)
        return STATUS_ERR(ENOMEM);

    while (current) {
        char* argument_string = process_malloc(process, sizeof(current->argument));

        if (!argument_string) {
            res = STATUS_ERR(ENOMEM);
            break;
        }

        strncpy(argument_string, current->argument, sizeof(current->argument));

        argv[index] = argument_string;
        current = current->next;
        index++;
    }

    if (status_is_error(res)) {
        for (int i = 0; i < index; i++) {
            kfree(argv[i]);
        }
        kfree(argv);
        return res;
    }

    process->arguments.argv = argv;
    process->arguments.argc = argc;
    
    return STATUS_OK;
}

static struct process_allocations* process_get_allocation_by_addr(struct process* process, void* address)
{
    for (int i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++) {
        if (process->allocations[i].ptr == address)
            return &process->allocations[i];
    }

    return NULL;
}

void process_free(struct process* process, void* ptr)
{
    struct process_allocations* alloc = process_get_allocation_by_addr(process, ptr);
    
    if (!alloc)
        return;

    if (status_is_error(
        paging_map_to(
            process->main_thread->page_directory,
            alloc->ptr,
            alloc->ptr,
            paging_align_address((char*)alloc->ptr + alloc->size),
            0
        )
    )) {
        return;
    }

    process_allocation_unjoin(process, ptr);
    
    kfree(ptr);
}

status_t process_switch(struct process* process)
{
    if (!process || !process->main_thread)
        return STATUS_ERR(EINVAL);

    current_process = process;
    return task_switch(process->main_thread);
}

status_t process_load_switch(const char* filename, struct process** process_out)
{
    status_t res = process_load(filename, process_out);
    if (status_is_error(res))
        return res;

    return process_switch(*process_out);
}
