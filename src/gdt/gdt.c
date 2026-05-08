#include "gdt/gdt.h"
#include "console/console.h"

void encode_gdt_entry(uint8_t* entry, struct gdt_structured source)
{
    /* S bit (bit 4 of access byte): 1 = code/data, 0 = system (e.g., TSS). */
    int is_code_or_data_segment = (source.type & 0x10) != 0;

    if ((source.limit > 0x100000) && (source.limit & 0xFFF) != 0xFFF) {
        panic("GDT entry limit must be either below 1MB or a multiple of 4KB minus 1");
    }

    if (source.limit > 0x100000) {
        source.limit = source.limit >> 12;
        entry[6] = is_code_or_data_segment ? 0xC0 : 0x80;
    } else {
        entry[6] = is_code_or_data_segment ? 0x40 : 0x00;
    }

    entry[0] = source.limit & 0xFF;
    entry[1] = (source.limit >> 8) & 0xFF;
    entry[6] |= (source.limit >> 16) & 0x0F;

    entry[2] = source.base & 0xFF;
    entry[3] = (source.base >> 8) & 0xFF;
    entry[4] = (source.base >> 16) & 0xFF;
    entry[7] = (source.base >> 24) & 0xFF;

    entry[5] = source.type;
}

void gdt_structured_to_gdt(struct gdt_entry* gdt, struct gdt_structured* structured, int entry_count)
{
    for (int i = 0; i < entry_count; i++) {
        encode_gdt_entry((uint8_t*)&gdt[i], structured[i]);
    }
}
