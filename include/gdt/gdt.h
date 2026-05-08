#ifndef GDT_H
#define GDT_H

#include <stdint.h>

/** Global Descriptor Table entry structure. */
struct gdt_entry
{
    /* Segment limit (bits 0-15). */
    uint16_t limit_low;
    /* Base address (bits 0-15). */
    uint16_t base_low;
    /* Base address (bits 16-23). */
    uint8_t base_middle;
    /* Access flags. */
    uint8_t access;
    /* Granularity and segment limit (bits 16-19). */
    uint8_t granularity;
    /* Base address (bits 24-31). */
    uint8_t base_high;
} __attribute__((packed));

struct gdt_structured
{
    /* Base address of the GDT. */
    uint32_t base;
    /* Limit of the GDT (size - 1). */
    uint32_t limit;
    /* Type of GDT entry (e.g. code/data). */
    uint8_t type;
} __attribute__((packed));

/**
 * Loads the GDT into the processor's descriptor table register.
 *
 * @param gdt GDT entry array and metadata to load.
 * @param size Total size of the GDT in bytes.
 * @return None.
 */
void gdt_load(struct gdt_entry* gdt, uint16_t size);

/**
 * Convert a structured GDT representation into raw GDT entries.
 *
 * @param gdt Output GDT entry array to populate.
 * @param structured Input structured GDT metadata and type.
 * @param entry_count Number of entries to generate in the output GDT.
 * @return None.
 */
void gdt_structured_to_gdt(struct gdt_entry* gdt, struct gdt_structured* structured, int entry_count);

#endif /* GDT_H */