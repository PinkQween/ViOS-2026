#include "gdt/gdt.h"
#include "memory/memory.h"

/* =========================================================
 * STANDARD GDT ENTRY
 * ========================================================= */

void gdt_set(
    struct gdt_entry* entry,
    void* address,
    uint32_t limit,
    uint8_t access,
    uint8_t flags
)
{
    memset(entry, 0, sizeof(struct gdt_entry));

    entry->limit_low = limit & 0xFFFF;

    entry->base_low = (uintptr_t)address & 0xFFFF;
    entry->base_middle = ((uintptr_t)address >> 16) & 0xFF;
    entry->base_high = ((uintptr_t)address >> 24) & 0xFF;

    entry->access = access;

    entry->limit_high_flags =
        ((limit >> 16) & 0x0F) |
        (flags & 0xF0);
}

/* =========================================================
 * 64-BIT TSS DESCRIPTOR
 * ========================================================= */

void gdt_set_tss(
    struct tss_desc_64* desc,
    void* tss_addr,
    uint32_t limit,
    uint8_t access,
    uint8_t flags
)
{
    memset(desc, 0, sizeof(struct tss_desc_64));

    uint64_t base = (uint64_t)tss_addr;

    desc->limit_low = limit & 0xFFFF;

    desc->base_low = base & 0xFFFF;
    desc->base_middle0 = (base >> 16) & 0xFF;
    desc->base_middle1 = (base >> 24) & 0xFF;
    desc->base_high = (base >> 32) & 0xFFFFFFFF;

    desc->access = access;

    desc->limit_high_flags =
        ((limit >> 16) & 0x0F) |
        (flags & 0xF0);
}
