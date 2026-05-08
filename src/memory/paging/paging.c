#include "memory/paging/paging.h"
#include "memory/heap/kheap.h"
#include "status.h"

#include <stdint.h>
#include <stdbool.h>

void paging_load_directory(uint32_t* directory);

static uint32_t* current_directory = 0;;

struct paging_4gb_chunk *paging_new_4gb(uint8_t flags)
{
    uint32_t *directory = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
    if (!directory) {
        return NULL;
    }

    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++)
    {
        uint32_t* entry = kzalloc(sizeof(uint32_t) * PAGING_TOTAL_ENTRIES_PER_TABLE);
        if (!entry) {
            for (int table = 0; table < i; table++) {
                kfree((void*)(directory[table] & 0xFFFFF000));
            }
            kfree(directory);
            return NULL;
        }

        for (int j = 0; j < PAGING_TOTAL_ENTRIES_PER_TABLE; j++)
        {
            uint32_t physical_address = ((i * PAGING_TOTAL_ENTRIES_PER_TABLE) + j) * PAGING_PAGE_SIZE_BYTES;
            entry[j] = physical_address | flags | PAGING_IS_PRESENT;
        }

        directory[i] = ((uint32_t)entry) | flags | PAGING_IS_WRITEABLE | PAGING_IS_PRESENT;
    }

    struct paging_4gb_chunk *chunk = kzalloc(sizeof(struct paging_4gb_chunk));
    if (!chunk) {
        for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++) {
            kfree((void*)(directory[i] & 0xFFFFF000));
        }
        kfree(directory);
        return NULL;
    }

    chunk->directory_entry = directory;

    return chunk;
}

void paging_switch(struct paging_4gb_chunk* directory)
{
    paging_load_directory(directory->directory_entry);
    current_directory = directory->directory_entry;
}

uint32_t* paging_4gb_chunk_get_directory(struct paging_4gb_chunk *chunk)
{
    return chunk->directory_entry;
}


bool paging_is_aligned(void* address)
{
    return ((uint32_t)address % PAGING_PAGE_SIZE_BYTES) == 0;
}

status_t paging_get_indexes(void* virtual_address, uint32_t* directory_index_out, uint32_t* table_index_out)
{
    status_t res = STATUS_OK;
    
    if (!paging_is_aligned(virtual_address))
    {
        res = STATUS_ERR(EINVAL);
        goto out;
    }

    *directory_index_out = ((uint32_t)virtual_address / (PAGING_PAGE_SIZE_BYTES * PAGING_TOTAL_ENTRIES_PER_TABLE));
    *table_index_out = ((uint32_t)virtual_address % (PAGING_PAGE_SIZE_BYTES * PAGING_TOTAL_ENTRIES_PER_TABLE) / PAGING_PAGE_SIZE_BYTES);

out:
    return res;
}

status_t paging_set(struct paging_4gb_chunk* directory, void* virtual_address, uint32_t value) {
    if (!paging_is_aligned(virtual_address)) {
        return STATUS_ERR(EINVAL);
    }

    uint32_t directory_index, table_index = 0;

    status_t res = paging_get_indexes(virtual_address, &directory_index, &table_index);

    if (status_is_error(res)) {
        return res;
    }

    uint32_t entry = ((uint32_t*)directory->directory_entry)[directory_index];
    uint32_t* table = (uint32_t*)(entry & 0xFFFFF000);
    table[table_index] = value;
    
    return STATUS_OK;
}

void paging_free_4gb(struct paging_4gb_chunk *chunk)
{
    if (!chunk) {
        return;
    }

    if (!chunk->directory_entry) {
        kfree(chunk);
        return;
    }

    for (int i = 0; i < PAGING_TOTAL_ENTRIES_PER_TABLE; i++)
    {
        uint32_t entry = chunk->directory_entry[i];
        if (entry & PAGING_IS_PRESENT)
        {
            uint32_t* table = (uint32_t*)(entry & 0xFFFFF000);
            kfree(table);
        }
    }

    kfree(chunk->directory_entry);
    kfree(chunk);
}

status_t paging_map(struct paging_4gb_chunk* directory, void* virtual_address, void* physical_address, uint8_t flags)
{
    if (!paging_is_aligned(virtual_address) || !paging_is_aligned(physical_address)) {
        return STATUS_ERR(EINVAL);
    }

    return paging_set(directory, virtual_address, ((uint32_t)physical_address) | flags);
}

status_t paging_map_range(struct paging_4gb_chunk* directory, void* virtual_address, void* physical_address, uint32_t total_pages, uint8_t flags)
{
    status_t res = STATUS_OK;

    for (uint32_t i = 0; i < total_pages; i++)
    {
        res = paging_map(directory, virtual_address, physical_address, flags);

        if (status_is_error(res)) break;

        virtual_address += PAGING_PAGE_SIZE_BYTES;
        physical_address += PAGING_PAGE_SIZE_BYTES;
    }

    return res;
}

status_t paging_map_to(struct paging_4gb_chunk* directory, void* virtual_address, void* physical_address, void* physical_address_end, uint8_t flags)
{
    if (!paging_is_aligned(virtual_address) || !paging_is_aligned(physical_address) || !paging_is_aligned(physical_address_end)) {
        return STATUS_ERR(EINVAL);
    }

    status_t res = STATUS_OK;

    if ((uint32_t)virtual_address % PAGING_PAGE_SIZE_BYTES) {
        res = STATUS_ERR(EINVAL);
        goto out;
    }

    if ((uint32_t)physical_address % PAGING_PAGE_SIZE_BYTES) {
        res = STATUS_ERR(EINVAL);
        goto out;
    }

    if ((uint32_t)physical_address_end % PAGING_PAGE_SIZE_BYTES) {
        res = STATUS_ERR(EINVAL);
        goto out;
    }

    if ((uint32_t)physical_address_end < (uint32_t)physical_address) {
        res = STATUS_ERR(EINVAL);
        goto out;
    }

    uint32_t total_bytes = (uint32_t)physical_address_end - (uint32_t)physical_address;
    uint32_t total_pages = total_bytes / PAGING_PAGE_SIZE_BYTES;

    res = paging_map_range(directory, virtual_address, physical_address, total_pages, flags);

out:
    return res;
}

void* paging_align_to_lower_page(void* address)
{
    return (void*)((uint32_t)address - ((uint32_t)address % PAGING_PAGE_SIZE_BYTES));
}

void* paging_align_address(void* address)
{
    if ((uint32_t)address % PAGING_PAGE_SIZE_BYTES == 0)
    {
        return address;
    }
    else
    {
        return (void*)(((uint32_t)address + PAGING_PAGE_SIZE_BYTES) - ((uint32_t)address % PAGING_PAGE_SIZE_BYTES));
    }
}

uint32_t paging_get(uint32_t* directory, void* virtual_address)
{
    if (!paging_is_aligned(virtual_address)) {
        return 0;
    }

    uint32_t directory_index, table_index = 0;

    status_t res = paging_get_indexes(virtual_address, &directory_index, &table_index);

    if (status_is_error(res)) {
        return 0;
    }

    uint32_t entry = directory[directory_index];

    if (!(entry & PAGING_IS_PRESENT)) {
        return 0;
    }

    uint32_t* table = (uint32_t*)(entry & 0xFFFFF000);
    return table[table_index];
}
