#ifndef TSS_H
#define TSS_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file tss.h
 * @brief Task State Segment (TSS) layout and setup API.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include "stdint.h"

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
    uint32_t reserved0;
    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;
    uint64_t reserved1;
    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;
    uint64_t reserved2;
    uint16_t reserved3;
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
