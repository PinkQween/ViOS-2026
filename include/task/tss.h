#ifndef TSS_H
#define TSS_H

#include <stdint.h>

/**
 * Task State Segment (TSS) structure
 *
 * The TSS is a special data structure used by the x86 architecture to hold information about a task. It is used for hardware task switching and for handling certain types of interrupts. The TSS contains information about the task's stack, registers, and other state information.
 * The fields in the TSS structure are as follows:
 * - `link`: The previous TSS selector (not used in modern operating systems).
 * - `esp0`: The stack pointer to load when switching to kernel mode.
 * - `ss0`: The stack segment to load when switching to kernel mode.
 * - `esp1`: The stack pointer to load when switching to ring 1 (not used in modern operating systems).
 * - `ss1`: The stack segment to load when switching to ring 1 (not used in modern operating systems).
 * - `esp2`: The stack pointer to load when switching to ring 2 (not used in modern operating systems).
 * - `ss2`: The stack segment to load when switching to ring 2 (not used in modern operating systems).
 * - `cr3`: The page directory base register (not used in modern operating systems).
 * - `eip`: The instruction pointer to load when switching to this task (not used in modern operating systems).
 * - `eflags`: The flags register to load when switching to this task (not used in modern operating systems).
 * - `eax`, `ecx`, `edx`, `ebx`, `esp`, `ebp`, `esi`, `edi`: The general-purpose registers to load when switching to this task (not used in modern operating systems).
 * - `es`, `cs`, `ss`, `ds`, `fs`, `gs`: The segment selectors to load when switching to this task (not used in modern operating systems).
 * - `ldt`: The Local Descriptor Table segment selector (not used in modern operating systems).
 * - `trap`: A flag indicating whether a trap should be generated on a task switch (not used in modern operating systems).
 * - `iomap_base`: The offset to the I/O permission bitmap (not used in modern operating systems).
 */
struct tss
{
    uint32_t link;
    uint32_t esp0;
    uint32_t ss0;
    uint32_t esp1;
    uint32_t ss1;
    uint32_t esp2;
    uint32_t ss2;
    uint32_t cr3;
    uint32_t eip;
    uint32_t eflags;
    uint32_t eax;
    uint32_t ecx;
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi;
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldt;
    uint16_t trap;
    uint16_t iomap_base;
} __attribute__((packed));

/**
 * Load the TSS segment selector into the Task Register (TR).
 *
 * @param selector The GDT selector index for the TSS segment.
 * @return None.
 */
void tss_load(uint16_t selector);

#endif // TSS_H