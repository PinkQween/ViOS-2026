#ifndef IDT_H
#define IDT_H

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

void idt_init(void);
void idt_load(struct idtr_desc* ptr);
void idt_unmask_irq(uint8_t irq);
void isr80h(void);
extern void* interrupt_pointer_table[TOTAL_INTERRUPTS];
void idt_handle_error(int interrupt, struct interrupt_frame* frame);
status_t idt_register_interrupt_callback(int interrupt, INTERRUPT_CALLBACK callback);
void* isr80h_register_command(int command, ISR80H_COMMAND handler);

#endif /* IDT_H */
