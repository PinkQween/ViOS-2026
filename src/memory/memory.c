#include "memory/memory.h"
#include "config.h"

size_t e820_total_entries()
{
    return *((uint16_t*)MEMORY_MAP_TOTAL_ENTRIES_LOCATION);
}

struct e820_entry* e820_get_entry(size_t index)
{
    if (index >= e820_total_entries())
    {
        return NULL;
    }

    return (struct e820_entry*)(MEMORY_MAP_LOCATION + (index * sizeof(struct e820_entry)));
}

struct e820_entry* e820_largest_free_entry(struct e820_entry* entries, size_t total_entries)
{
    if (!entries || total_entries == 0)
    {
        total_entries = e820_total_entries();
        entries = (struct e820_entry*)MEMORY_MAP_LOCATION;
    }

    struct e820_entry* chosen_entry = NULL;

    for (size_t i = 0; i < total_entries; i++)
    {
        struct e820_entry* entry = &entries[i];

        if (entry->type == 1)
        {
            if (!chosen_entry)
            {
                chosen_entry = entry;
                continue;
            }

            if (entry->length > chosen_entry->length)
            {
                chosen_entry = entry;
            }
        }
    }

    return chosen_entry;
}

size_t e820_total_accessible_memory(struct e820_entry* entries, size_t total_entries)
{
    if (!entries || total_entries == 0)
    {
        total_entries = e820_total_entries();
        entries = (struct e820_entry*)MEMORY_MAP_LOCATION;
    }

    size_t total_accessible_memory = 0;

    for (size_t i = 0; i < total_entries; i++)
    {
        struct e820_entry* entry = &entries[i];

        if (entry->type == 1)
        {
            total_accessible_memory += entry->length;
        }
    }

    return total_accessible_memory;
}

void *memset(void *ptr, int c, size_t size)
{
    char *c_ptr = (char *)ptr;
    for (size_t i = 0; i < size; i++)
    {
        c_ptr[i] = (char)c;
    }
    return ptr;
}

void *memcpy(void *dest, const void *src, size_t size)
{
    char *c_dest = (char *)dest;
    const char *c_src = (const char *)src;
    for (size_t i = 0; i < size; i++)
    {
        c_dest[i] = c_src[i];
    }
    return dest;
}

bool safe_memcpy(void *dest, size_t dest_size, const void *src, size_t size)
{
    if (!dest || !src || size > dest_size) {
        return false;
    }

    memcpy(dest, src, size);
    return true;
}

int memcmp(const void *ptr1, const void *ptr2, size_t count)
{
    const char *c_ptr1 = (const char *)ptr1;
    const char *c_ptr2 = (const char *)ptr2;

    for (size_t i = 0; i < count; i++)
    {
        if (c_ptr1[i] != c_ptr2[i])
        {
            return (int)(unsigned char)c_ptr1[i] - (int)(unsigned char)c_ptr2[i];
        }
    }

    return 0;
}
