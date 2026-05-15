/**
 * Copyright (c) 2026 Hanna Skairipa,
 * 
 * @file process.c
 * @breif Kernel process management implementation.
 */

#include "task/process.h"
#include "task/task.h"

#include "config.h"
#include "status.h"

#include "memory/heap/kheap.h"
#include "memory/heap/heap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"

#include "fs/file.h"

#include "loader/elf/elfloader.h"
#include "loader/elf/elf.h"

#include "string/string.h"

#include "console/console.h"

#include "vector/vector.h"

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"


struct process* current_process = NULL;
struct process* processes[MAX_PROCESSES] = {0};

void* process_virtual_address_to_physical(
    struct process* process,
    void* virt_addr
)
{
    return paging_get_physical_address(
        process->paging_desc,
        virt_addr
    );
}

static void process_init(struct process* process)
{
    memset(process, 0, sizeof(struct process));
    process->file_handles = vector_new(sizeof(struct process_file_handle*), 4, 0);
}

struct process* process_current()
{
    return current_process;
}

struct process* process_get(int pid)
{
    if (pid < 0 || pid >= MAX_PROCESSES)
    {
        return NULL;
    }

    return processes[pid];
}

static status_t process_load_binary(
    const char* filename,
    struct process* process
)
{
    status_t fd = fopen(filename, "rb");

    if (status_is_error(fd))
    {
        return fd;
    }

    struct file_stat stat;

    status_t res = fstat(fd, &stat);

    if (status_is_error(res))
    {
        fclose(fd);
        return res;
    }

    size_t aligned_size = heap_align_value_to_upper((size_t)stat.size);

    void* program_data = kpzalloc(aligned_size);

    if (!program_data)
    {
        fclose(fd);
        return STATUS_ERR(ENOMEM);
    }

    res = fread(program_data, 1, stat.size, fd);

    if (status_is_error(res) || res != stat.size)
    {
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

static status_t process_load_elf(
    const char* filename,
    struct process* process
)
{
    struct elf_file* elf = NULL;

    status_t res = elf_load(filename, &elf);

    if (status_is_error(res))
    {
        return res;
    }

    res = elf_load_needed_libraries(elf);

    if (status_is_error(res))
    {
        elf_close(elf);
        return res;
    }

    res = elf_apply_relocations(elf);

    if (status_is_error(res))
    {
        elf_close(elf);
        return res;
    }

    process->elf = elf;
    process->filetype = PROCESS_TYPE_ELF;

    return STATUS_OK;
}

static status_t process_load_data(
    const char* filename,
    struct process* process
)
{
    status_t res = process_load_elf(filename, process);

    if (status_is_ok(res))
    {
        return res;
    }

    return process_load_binary(filename, process);
}

static status_t process_map_binary(struct process* process)
{
    void* phys_end = paging_align_address((void*)((uintptr_t)process->ptr + (uintptr_t)process->size));

    return paging_map_to(
        process->paging_desc,
        (void*)PROGRAM_VIRTUAL_ADDRESS,
        process->ptr,
        phys_end,
        PAGING_IS_WRITEABLE |
        PAGING_IS_PRESENT |
        PAGING_ACCESSIBLE_FROM_ALL
    );
}

static status_t process_map_elf_file(
    struct process* process,
    struct elf_file* elf_file
)
{
    for (int i = 0; i < elf_file->segment_count; i++)
    {
        struct elf_segment* seg =
            &elf_file->segments[i];

        int flags =
            PAGING_IS_PRESENT |
            PAGING_ACCESSIBLE_FROM_ALL;

        if (seg->flags & PF_W)
        {
            flags |= PAGING_IS_WRITEABLE;
        }

        void* phys_start = paging_align_to_lower_page(seg->paddr);
        void* phys_end = paging_align_address((void*)((uintptr_t)seg->paddr + (uintptr_t)seg->memsz));
        void* virt_start = paging_align_to_lower_page(seg->vaddr);

        if (!seg->paddr)
        {
            print("ERROR: Segment has NULL paddr!\n");
            return STATUS_ERR(EINVAL);
        }

        status_t res = paging_map_to(
            process->paging_desc,
            virt_start,
            phys_start,
            phys_end,
            flags
        );

        if (status_is_error(res))
        {
            return res;
        }

        if (seg->memsz > seg->filesz)
        {
            memset(
                (char*)seg->paddr + seg->filesz,
                0,
                seg->memsz - seg->filesz
            );
        }
    }

    for (int i = 0; i < elf_file->needed_count; i++)
    {
        status_t res = process_map_elf_file(
            process,
            elf_file->needed_libraries[i]
        );

        if (status_is_error(res))
        {
            return res;
        }
    }

    return STATUS_OK;
}

static status_t process_map_elf(struct process* process)
{
    if (!process || !process->elf)
    {
        return STATUS_ERR(EINVAL);
    }

    return process_map_elf_file(
        process,
        process->elf
    );
}

status_t process_map_memory(struct process* process)
{
    if (!process)
    {
        return STATUS_ERR(EINVAL);
    }

    paging_map_e820_memory_regions(
        process->paging_desc
    );

    status_t res = STATUS_OK;

    switch (process->filetype)
    {
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
    {
        return res;
    }

    return paging_map_to(
        process->paging_desc,
        (void*)PROGRAM_VIRTUAL_STACK_ADDRESS_END,
        process->stack,
        paging_align_address((void*)((uintptr_t)process->stack + (uintptr_t)USER_PROGRAM_STACK_SIZE)),
        PAGING_IS_WRITEABLE |
        PAGING_IS_PRESENT |
        PAGING_ACCESSIBLE_FROM_ALL
    );
}

status_t process_get_free_slot()
{
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (!processes[i])
        {
            return i;
        }
    }

    return STATUS_ERR(EBUSY);
}

status_t process_load_for_slot(
    const char* filename,
    struct process** process_out,
    int process_slot
)
{
    if (!filename || !process_out)
    {
        return STATUS_ERR(EINVAL);
    }

    if (process_get(process_slot))
    {
        return STATUS_ERR(EBUSY);
    }

    status_t res = STATUS_OK;

    struct process* new_process =
        kzalloc(sizeof(struct process));

    if (!new_process)
    {
        return STATUS_ERR(ENOMEM);
    }

    process_init(new_process);

    res = process_load_data(filename, new_process);

    if (status_is_error(res))
    {
        goto fail;
    }

    new_process->stack =
        kpzalloc(USER_PROGRAM_STACK_SIZE);

    if (!new_process->stack)
    {
        res = STATUS_ERR(ENOMEM);
        goto fail;
    }

    strncpy(
        new_process->filename,
        filename,
        sizeof(new_process->filename)
    );

    new_process->pid = process_slot;

    new_process->paging_desc =
        paging_desc_new(PAGING_MAP_LEVEL_4);

    if (!new_process->paging_desc)
    {
        res = STATUS_ERR(EIO);
        goto fail;
    }

    res = process_map_memory(new_process);

    if (status_is_error(res))
    {
        goto fail;
    }

    new_process->main_thread =
        task_new(new_process);

    if (!new_process->main_thread)
    {
        res = STATUS_ERR(ENOMEM);
        goto fail;
    }

    if (new_process->filetype == PROCESS_TYPE_ELF)
    {
        // TODO: Implement 32-bit ELF process execution support
        // Need to handle mode switching, compatibility mode, and 32-bit address space
        if (new_process->elf->elf_class == ELFCLASS32)
        {
            res = STATUS_ERR(ENOSYS);
            goto fail;
        }

        new_process->main_thread->registers.rip =
            (uint64_t)elf_entry_point(
                new_process->elf
            );
    }

    // Initialize default arguments (program name only)
    void* argv_user = process_malloc(new_process, sizeof(char*) * 2);
    if (argv_user)
    {
        char** argv_kernel = (char**)process_malloc_get_kernel_ptr(new_process, argv_user);
        size_t name_len = strlen(filename) + 1;
        void* arg0_user = process_malloc(new_process, name_len);
        if (arg0_user)
        {
            char* arg0_kernel = (char*)process_malloc_get_kernel_ptr(new_process, arg0_user);
            strncpy(arg0_kernel, filename, name_len);
            argv_kernel[0] = (char*)arg0_user;
            argv_kernel[1] = NULL;
            new_process->arguments.argc = 1;
            new_process->arguments.argv = (char**)argv_user;
        }
        else
        {
            process_free(new_process, argv_user);
        }
    }

    *process_out = new_process;

    processes[process_slot] = new_process;

    return STATUS_OK;

fail:

    if (new_process->main_thread)
    {
        task_free(new_process->main_thread);
    }

    if (new_process->paging_desc)
    {
        paging_desc_free(
            new_process->paging_desc
        );
    }

    if (new_process->stack)
    {
        kfree(new_process->stack);
    }

    if (new_process->ptr)
    {
        kfree(new_process->ptr);
    }

    if (new_process->elf)
    {
        elf_close(new_process->elf);
    }

    kfree(new_process);

    return res;
}

status_t process_load(
    const char* filename,
    struct process** process_out
)
{
    status_t slot = process_get_free_slot();

    if (status_is_error(slot))
    {
        return STATUS_ERR(EBUSY);
    }

    return process_load_for_slot(
        filename,
        process_out,
        slot
    );
}

static status_t process_find_free_allocation_index(
    struct process* process
)
{
    for (int i = 0;
         i < MAX_PROGRAM_ALLOCATIONS;
         i++)
    {
        if (!process->allocations[i].user_ptr)
        {
            return i;
        }
    }

    return STATUS_ERR(ENOMEM);
}

void* process_malloc(
    struct process* process,
    size_t size
)
{
    void* kernel_ptr = kzalloc(size);

    if (!kernel_ptr)
    {
        return NULL;
    }

    int index =
        process_find_free_allocation_index(process);

    if (status_is_error(index))
    {
        kfree(kernel_ptr);
        return NULL;
    }

    uintptr_t user_vaddr_int =
        0x500000UL +
        ((uintptr_t)index * 0x10000UL);

    void* user_vaddr =
        (void*)user_vaddr_int;

    status_t res = paging_map_to(
        process->paging_desc,
        user_vaddr,
        kernel_ptr,
            paging_align_address((void*)((uintptr_t)kernel_ptr + (uintptr_t)size)),
        PAGING_IS_WRITEABLE |
        PAGING_IS_PRESENT |
        PAGING_ACCESSIBLE_FROM_ALL
    );

    if (status_is_error(res))
    {
        kfree(kernel_ptr);
        return NULL;
    }

    process->allocations[index].user_ptr =
        user_vaddr;

    process->allocations[index].kernel_ptr =
        kernel_ptr;

    process->allocations[index].size =
        size;

    return user_vaddr;
}

void* process_malloc_get_kernel_ptr(struct process* process, void* user_ptr)
{
    for (int i = 0; i < MAX_PROGRAM_ALLOCATIONS; i++)
    {
        if (process->allocations[i].user_ptr == user_ptr)
        {
            return process->allocations[i].kernel_ptr;
        }
    }

    return NULL;
}

static void process_allocation_unjoin(
    struct process* process,
    void* ptr
)
{
    for (int i = 0;
         i < MAX_PROGRAM_ALLOCATIONS;
         i++)
    {
        if (process->allocations[i].user_ptr == ptr)
        {
            process->allocations[i].user_ptr = NULL;
            process->allocations[i].kernel_ptr = NULL;
            process->allocations[i].size = 0;
        }
    }
}

void process_terminate_allocations(
    struct process* process
)
{
    for (int i = 0;
         i < MAX_PROGRAM_ALLOCATIONS;
         i++)
    {
        process_free(
            process,
            process->allocations[i].user_ptr
        );
    }
}

void process_free_binary_data(
    struct process* process
)
{
    if (process->ptr)
    {
        kfree(process->ptr);
        process->ptr = NULL;
    }
}

void process_free_elf_data(
    struct process* process
)
{
    if (process->elf)
    {
        elf_close(process->elf);
        process->elf = NULL;
    }
}

status_t process_close_file_handles(struct process* process)
{
    if (!process || !process->file_handles)
    {
        return STATUS_ERR(EINVAL);
    }

    size_t total_handles = vector_count(process->file_handles);
    
    for (size_t i = 0; i < total_handles; i++)
    {
        struct process_file_handle* handle = NULL;

        vector_at(process->file_handles, i, &handle, sizeof(struct process_file_handle*));

        if (handle)
        {
            fclose(handle->fd);
            kfree(handle);
        }
    }

    vector_free(process->file_handles);
    process->file_handles = NULL;

    return STATUS_OK;
}

status_t process_free_program_data(
    struct process* process
)
{
    switch (process->filetype)
    {
        case PROCESS_TYPE_ELF:
            process_free_elf_data(process);
            break;

        case PROCESS_TYPE_BINARY:
            process_free_binary_data(process);
            break;

        default:
            return STATUS_ERR(EINVAL);
    }

    return STATUS_OK;
}

void process_switch_to_any()
{
    for (int i = 0; i < MAX_PROCESSES; i++)
    {
        if (processes[i])
        {
            process_switch(processes[i]);
            return;
        }
    }

    panic("No processes available\n");
}

static void process_unlink(
    struct process* process
)
{
    processes[process->pid] = NULL;

    if (current_process == process)
    {
        process_switch_to_any();
    }
}

status_t process_terminate(
    struct process* process
)
{
    process_terminate_allocations(process);

    status_t res =
        process_free_program_data(process);

    if (status_is_error(res))
    {
        return res;
    }

    res = process_close_file_handles(process);


    if (status_is_error(res))
    {
        return res;
    }

    if (process->paging_desc)
    {
        paging_desc_free(
            process->paging_desc
        );
    }

    if (process->stack)
    {
        kfree(process->stack);
    }

    if (process->main_thread)
    {
        task_free(process->main_thread);
    }

    process_unlink(process);

    kfree(process);

    return STATUS_OK;
}

void process_get_arguments(
    struct process* process,
    int* argc,
    char*** argv
)
{
    if (!process || !argc || !argv)
    {
        return;
    }

    *argc = process->arguments.argc;
    *argv = process->arguments.argv;
}

int process_count_command_arguments(
    struct command_argument* root_arg
)
{
    int count = 0;

    struct command_argument* current =
        root_arg;

    while (current)
    {
        count++;
        current = current->next;
    }

    return count;
}

status_t process_inject_arguments(
    struct process* process,
    struct command_argument* root_arg
)
{
    status_t res = STATUS_OK;

    int argc =
        process_count_command_arguments(root_arg);

    if (argc == 0)
    {
        return STATUS_ERR(EINVAL);
    }

    void* argv_user = process_malloc(
        process,
        sizeof(char*) * (argc + 1)
    );

    if (!argv_user)
    {
        return STATUS_ERR(ENOMEM);
    }

    char** argv_kernel = (char**)process_malloc_get_kernel_ptr(process, argv_user);

    struct command_argument* current =
        root_arg;

    int index = 0;

    while (current)
    {
        size_t len =
            strlen(current->argument) + 1;

        void* argument_string_user =
            process_malloc(process, len);

        if (!argument_string_user)
        {
            res = STATUS_ERR(ENOMEM);
            break;
        }

        char* argument_string_kernel = (char*)process_malloc_get_kernel_ptr(process, argument_string_user);

        strncpy(
            argument_string_kernel,
            current->argument,
            len
        );

        argv_kernel[index++] = (char*)argument_string_user;

        current = current->next;
    }

    argv_kernel[index] = NULL;

    if (status_is_ok(res))
    {
        struct command_argument* debug_arg = root_arg;
        int debug_i = 0;
        while (debug_arg && debug_i < 5)
        {
            debug_arg = debug_arg->next;
            debug_i++;
        }

        process->arguments.argc = argc;
        process->arguments.argv = (char**)argv_user;
    }
    else
    {
        for (int i = 0; i < index; i++)
        {
            process_free(process, argv_kernel[i]);
        }

        process_free(process, argv_user);
    }

    return res;
}

static struct process_allocation*
process_get_allocation_by_addr(
    struct process* process,
    void* address
)
{
    for (int i = 0;
         i < MAX_PROGRAM_ALLOCATIONS;
         i++)
    {
        if (process->allocations[i].user_ptr
            == address)
        {
            return &process->allocations[i];
        }
    }

    return NULL;
}

void process_free(
    struct process* process,
    void* ptr
)
{
    struct process_allocation* alloc =
        process_get_allocation_by_addr(
            process,
            ptr
        );

    if (!alloc)
    {
        return;
    }

    paging_map_to(
        process->paging_desc,
        alloc->user_ptr,
        alloc->kernel_ptr,
        paging_align_address(
            (void*)((uintptr_t)alloc->kernel_ptr + alloc->size)
        ),
        0
    );

    process_allocation_unjoin(process, ptr);

    kfree(alloc->kernel_ptr);
}

status_t process_switch(struct process* process)
{
    if (!process || !process->main_thread)
    {
        return STATUS_ERR(EINVAL);
    }

    current_process = process;

    paging_switch(
        process->paging_desc
    );

    return task_switch(process->main_thread);
}

status_t process_load_switch(
    const char* filename,
    struct process** process_out
)
{
    status_t res =
        process_load(filename, process_out);

    if (status_is_error(res))
    {
        return res;
    }

    return process_switch(*process_out);
}

status_t process_fopen(struct process* process, const char* filepath, const char* mode)
{
    if (!process || !filepath || !mode)
    {
        return STATUS_ERR(EINVAL);
    }

    status_t fd = fopen(filepath, mode);

    if (status_is_error(fd))
    {
        return fd;
    }

    struct process_file_handle* handle = kzalloc(sizeof(struct process_file_handle));

    if (!handle)
    {
        fclose(fd);
        return STATUS_ERR(ENOMEM);
    }

    handle->fd = fd;
    strncpy(handle->filepath, filepath, sizeof(handle->filepath));
    strncpy(handle->mode, mode, sizeof(handle->mode));

    vector_push(process->file_handles, &handle);

    return fd;
}

struct process_file_handle* process_get_file_handle(struct process* process, int fd)
{
    if (!process || !process->file_handles)
    {
        return NULL;
    }

    size_t total_handles = vector_count(process->file_handles);

    for (size_t i = 0; i < total_handles; i++)
    {
        struct process_file_handle* handle = NULL;

        vector_at(process->file_handles, i, &handle, sizeof(struct process_file_handle*));

        if (handle && handle->fd == fd)
        {
            return handle;
        }
    }

    return NULL;
}