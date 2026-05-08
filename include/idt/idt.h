#ifndef IDT_H
#define IDT_H

#include "status.h"

#include <stdint.h>

struct interrupt_frame;
typedef void*(*ISR80H_COMMAND)(struct interrupt_frame* frame);
typedef void(*INTERRUPT_CALLBACK)(struct interrupt_frame* frame);

/**
 * Interrupt descriptor table gate entry.
 */
struct idt_desc
{
    /** Handler offset bits 0..15. */
    uint16_t offset_1;
    /** Code segment selector. */
    uint16_t selector;
    /** Reserved byte, must be zero. */
    uint8_t zero;
    /** Type and attribute flags. */
    uint8_t type_attr;
    /** Handler offset bits 16..31. */
    uint16_t offset_2;
} __attribute__((packed));

/**
 * IDTR register descriptor used by lidt.
 */
struct idtr_desc
{
    /** Size of IDT in bytes minus one. */
    uint16_t limit;
    /** Base address of IDT table. */
    uint32_t base;
} __attribute__((packed));

/**
 * Interrupt frame structure.
 */
struct interrupt_frame
{
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t reserved;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t esp;
    uint32_t ss;
} __attribute__((packed));

/**
 * Build and load the interrupt descriptor table.
 *
 * @return None.
 */
void idt_init();

status_t idt_register_interrupt_callback(int interrupt, INTERRUPT_CALLBACK callback);

/**
 * Set an entry in the IDT to point to a handler function.
 *
 * @param interrupt_number The interrupt vector number to set (0-255).
 * @param address The address of the handler function to set for this interrupt.
 * @return None.
 */
void* isr80h_register_command(int command, ISR80H_COMMAND handler);

#endif /* IDT_H */
