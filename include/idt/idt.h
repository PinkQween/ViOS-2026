#ifndef IDT_H
#define IDT_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file idt.h
 * @brief Interrupt Descriptor Table (IDT) definitions and interrupt API.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include "stdint.h"
#include "status.h" 
#include "config.h"

struct interrupt_frame; 

typedef void* (*ISR80H_COMMAND)(struct interrupt_frame*);
typedef void  (*INTERRUPT_CALLBACK)(struct interrupt_frame*);

struct idt_desc {
    uint16_t offset_1;
    uint16_t selector;
    uint8_t ist;
    uint8_t type_attr;
    uint16_t offset_2;
    uint32_t offset_3;
    uint32_t zero;
} __attribute__((packed));

struct idtr_desc {
    uint16_t limit;
    uint64_t base;
} __attribute__((packed));

struct interrupt_frame {
    uint64_t rdi, rsi, rbp, rsp, rbx, rdx, rcx, rax;
    uint64_t error_code;
    uint64_t rip, cs, rflags, user_rsp, ss;
} __attribute__((packed));

/**
 * Initialize the Interrupt Descriptor Table and interrupt handlers.
 *
 * @return None.
 */
void idt_init(void);

/**
 * Load an IDT descriptor with lidt.
 *
 * @param ptr IDT pointer/limit descriptor.
 * @return None.
 */
void idt_load(struct idtr_desc* ptr);

/**
 * Unmask a legacy PIC IRQ line.
 *
 * @param irq IRQ number to enable.
 * @return None.
 */
void idt_unmask_irq(uint8_t irq);

/**
 * Assembly ISR entry point for syscall interrupt 0x80.
 *
 * @return None.
 */
void isr80h(void);

extern void* interrupt_pointer_table[TOTAL_INTERRUPTS];

/**
 * Handle a CPU exception or interrupt error path.
 *
 * @param interrupt Interrupt vector number.
 * @param frame Saved interrupt frame.
 * @return None.
 */
void idt_handle_error(int interrupt, struct interrupt_frame* frame);

/**
 * Register a callback for an interrupt vector.
 *
 * @param interrupt Interrupt vector number.
 * @param callback Callback to invoke when the vector fires.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t idt_register_interrupt_callback(int interrupt, INTERRUPT_CALLBACK callback);

/**
 * Register an ISR 0x80 system call command handler.
 *
 * @param command System command number.
 * @param handler Handler function for the command.
 * @return Previously registered handler, or NULL if none existed.
 */
void* isr80h_register_command(int command, ISR80H_COMMAND handler);

#endif /* IDT_H */
