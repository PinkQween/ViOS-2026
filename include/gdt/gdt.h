#ifndef GDT_H
#define GDT_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file gdt.h
 * @brief Global Descriptor Table (GDT) definitions and setup API.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include "stdint.h"

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

/**
 * Populate a standard GDT segment descriptor.
 *
 * @param gdt_entry Descriptor to write.
 * @param address Segment base address.
 * @param limit Segment limit.
 * @param access_byte Access/type flags.
 * @param flags Granularity and size flags.
 * @return None.
 */
void gdt_set(struct gdt_entry* gdt_entry, void* address, uint32_t limit, uint8_t access_byte, uint8_t flags);

/**
 * Populate a 64-bit Task State Segment descriptor.
 *
 * @param desc Descriptor to write.
 * @param tss_addr TSS base address.
 * @param limit TSS limit.
 * @param access_byte Access/type flags.
 * @param flags Granularity and size flags.
 * @return None.
 */
void gdt_set_tss(struct tss_desc_64* desc, void* tss_addr, uint32_t limit, uint8_t access_byte, uint8_t flags);

/**
 * Load the Global Descriptor Table register.
 *
 * @param gdt Pointer/limit pair describing the GDT.
 * @return None.
 */
void gdt_load(struct gdt_ptr* gdt);

#endif /* GDT_H */
