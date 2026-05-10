#ifndef GDT_H
#define GDT_H

#include "stdint.h"

/**
 * @file gdt.h
 * @brief Global Descriptor Table (GDT) definitions and functions.
 *
 * This header defines the structures and functions for managing the
 * Global Descriptor Table (GDT) in an x86_64 operating system kernel.
 *
 * The GDT is a critical data structure used by the CPU to define the
 * characteristics of memory segments, including their base address,
 * limit, access rights, and other attributes. It is essential for
 * setting up protected mode and long mode operation.
 *
 * This implementation provides basic support for:
 * - Code and data segments
 * - Task State Segment (TSS) descriptors
 *
 * Note: This GDT implementation is simplified and may not cover all
 * possible segment types or features. It is intended for educational
 * purposes and may need to be extended for a full-featured kernel.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

struct gdt_entry
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle;
    uint8_t access;
    uint8_t limit_high_flags;
    uint8_t base_high;
} __attribute__((packed));

struct tss_desc_64
{
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t base_middle0;
    uint8_t access;
    uint8_t limit_high_flags;
    uint8_t base_middle1;
    uint32_t base_high;
    uint32_t reserved;
} __attribute__((packed));

struct gdt_ptr
{
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

void gdt_set(struct gdt_entry* gdt_entry, void* address, uint32_t limit, uint8_t access_byte, uint8_t flags);
void gdt_set_tss(struct tss_desc_64* desc, void* tss_addr, uint32_t limit, uint8_t access_byte, uint8_t flags);
void gdt_load(struct gdt_ptr* gdt);

#endif /* GDT_H */
