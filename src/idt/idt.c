#include "idt/idt.h"
#include "config.h"
#include "console/console.h"
#include "io/io.h"
#include "kernel.h"
#include "memory/memory.h"
#include "task/task.h"
#include "task/process.h"
#include "string/string.h"

#include "stdint.h"

static ISR80H_COMMAND isr80h_commands[MAX_ISR80H_COMMANDS];
static INTERRUPT_CALLBACK interrupt_callbacks[TOTAL_INTERRUPTS];
static uint8_t pic_master_mask = 0xFF;
static uint8_t pic_slave_mask = 0xFF;

struct idt_desc idt_descriptors[TOTAL_INTERRUPTS];
struct idtr_desc idtr_descriptor;

static void idt_io_wait(void)
{
    outb(0x80, 0);
}

static void idt_apply_pic_masks(void)
{
    outb(0x21, pic_master_mask);
    outb(0xA1, pic_slave_mask);
}

static void idt_remap_pic(void)
{
    pic_master_mask = 0xFF;
    pic_slave_mask = 0xFF;

    outb(0x21, pic_master_mask);
    outb(0xA1, pic_slave_mask);

    outb(0x20, 0x11);
    idt_io_wait();
    outb(0xA0, 0x11);
    idt_io_wait();
    outb(0x21, 0x20);
    idt_io_wait();
    outb(0xA1, 0x28);
    idt_io_wait();
    outb(0x21, 0x04);
    idt_io_wait();
    outb(0xA1, 0x02);
    idt_io_wait();
    outb(0x21, 0x01);
    idt_io_wait();
    outb(0xA1, 0x01);
    idt_io_wait();

    idt_apply_pic_masks();
}

void idt_unmask_irq(uint8_t irq)
{
    if (irq >= 16) {
        return;
    }

    if (irq < 8) {
        pic_master_mask &= (uint8_t)~(1 << irq);
    } else {
        pic_master_mask &= (uint8_t)~(1 << 2);
        pic_slave_mask &= (uint8_t)~(1 << (irq - 8));
    }

    idt_apply_pic_masks();
}

static void idt_end_of_interrupt(int interrupt)
{
    if (interrupt < 0x20 || interrupt > 0x2f) {
        return;
    }

    if (interrupt >= 0x28) {
        outb(0xa0, 0x20);
    }
    outb(0x20, 0x20);
}

static const char* idt_exception_name(int interrupt)
{
    static const char* names[] = {
        "Divide Error",
        "Debug",
        "Non-maskable Interrupt",
        "Breakpoint",
        "Overflow",
        "Bound Range Exceeded",
        "Invalid Opcode",
        "Device Not Available",
        "Double Fault",
        "Coprocessor Segment Overrun",
        "Invalid TSS",
        "Segment Not Present",
        "Stack-Segment Fault",
        "General Protection Fault",
        "Page Fault",
        "Reserved",
        "x87 Floating-Point Exception",
        "Alignment Check",
        "Machine Check",
        "SIMD Floating-Point Exception",
        "Virtualization Exception",
        "Control Protection Exception"
    };

    if (interrupt >= 0 && interrupt < (int)(sizeof(names) / sizeof(names[0]))) {
        return names[interrupt];
    }

    return "Reserved Exception";
}

static uint32_t idt_page_fault_address(void)
{
    uint32_t address;
    __asm__ __volatile__("mov %%cr2, %0" : "=r"(address));
    return address;
}

static void idt_format_exception_message(char* message, size_t size, int interrupt, struct interrupt_frame* frame)
{
    if (interrupt == 14) {
        snprintf(
            message,
            size,
            "%s cr2=0x%x err=0x%x eip=0x%x eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
            idt_exception_name(interrupt),
            idt_page_fault_address(),
            frame ? frame->error_code : 0,
            frame ? frame->ip : 0,
            frame ? frame->eax : 0,
            frame ? frame->ebx : 0,
            frame ? frame->ecx : 0,
            frame ? frame->edx : 0
        );
        return;
    }

    snprintf(
        message,
        size,
        "%s err=0x%x eip=0x%x eax=0x%x ebx=0x%x ecx=0x%x edx=0x%x\n",
        idt_exception_name(interrupt),
        frame ? frame->error_code : 0,
        frame ? frame->ip : 0,
        frame ? frame->eax : 0,
        frame ? frame->ebx : 0,
        frame ? frame->ecx : 0,
        frame ? frame->edx : 0
    );
}

void idt_handle_error(int interrupt, struct interrupt_frame* frame)
{
    char message[256];
    idt_format_exception_message(message, sizeof(message), interrupt, frame);

    if (interrupt == 8 || !task_current() || !task_current()->process) {
        panic(message);
    }

    print_w_color("Terminating process after exception: ", choose_colour(LIGHT_RED, BLACK));
    print_w_color(message, choose_colour(LIGHT_RED, BLACK));

    process_terminate(task_current()->process);
    task_next();
}

static void idt_clock(struct interrupt_frame* frame)
{
    (void)frame;
    idt_end_of_interrupt(0x20);
    task_next();
}

void interrupt_handler(int interrupt, struct interrupt_frame* frame)
{
    kernel_page();

    INTERRUPT_CALLBACK callback = 0;
    if (interrupt >= 0 && interrupt < TOTAL_INTERRUPTS) {
        callback = interrupt_callbacks[interrupt];
        if (callback && task_current()) {
            task_current_save_state(frame);
            callback(frame);
        }
    }    

    if (!callback && interrupt >= 0 && interrupt < 0x20) {
        idt_handle_error(interrupt, frame);
    }

    task_page();
    idt_end_of_interrupt(interrupt);
}

status_t idt_register_interrupt_callback(int interrupt, INTERRUPT_CALLBACK callback)
{
    if (interrupt < 0 || interrupt >= TOTAL_INTERRUPTS) {
        return STATUS_ERR(EINVAL);
    }

    if (interrupt_callbacks[interrupt]) {
        return STATUS_ERR(EEXIST);
    }

    interrupt_callbacks[interrupt] = callback;
    return STATUS_OK;
}

static void idt_set(int interrupt_number, uint32_t address, uint8_t type_attr)
{
    struct idt_desc *desc = &idt_descriptors[interrupt_number];
    desc->offset_1 = (uint32_t)address & 0xFFFF;
    desc->selector = KERNEL_CODE_SELECTOR;
    desc->zero = 0;
    desc->type_attr = type_attr;
    desc->offset_2 = (uint32_t)address >> 16;
}

void idt_init()
{
    memset(idt_descriptors, 0, sizeof(idt_descriptors));
    idtr_descriptor.limit = sizeof(idt_descriptors) - 1;
    idtr_descriptor.base = (uint32_t)idt_descriptors;
    
    for (int i = 0; i < TOTAL_INTERRUPTS; i++)
    {
        idt_set(i, (uint32_t)interrupt_pointer_table[i], 0x8E);
    }

    idt_set(0x80, (uint32_t)isr80h, 0xEE);
    idt_remap_pic();

    status_t res = idt_register_interrupt_callback(0x20, idt_clock);
    if (status_is_error(res)) {
        panic_status("Failed to register PIC timer interrupt", res);
    }

    idt_load(&idtr_descriptor);
}

void* isr80h_handle_command(int command, struct interrupt_frame* frame)
{
    void* res = 0;

    if (command < 0 || command >= MAX_ISR80H_COMMANDS) {
        return res;
    }

    ISR80H_COMMAND cmd_handler = isr80h_commands[command];
    
    if (cmd_handler) {
        res = cmd_handler(frame);
    }

    return res;
}

void* isr80h_handler(int command, struct interrupt_frame* frame)
{
    void* res = 0;

    kernel_page();
    task_current_save_state(frame);

    res = isr80h_handle_command(command, frame);

    task_page();

    return res;
}

void* isr80h_register_command(int command, ISR80H_COMMAND handler)
{
    if (command < 0 || command >= MAX_ISR80H_COMMANDS) {
        panic("Attempted to register out-of-bounds isr80h command handler");
    }

    if (isr80h_commands[command]) {
        panic("Attempted to register isr80h command handler for already-registered command");
    }

    isr80h_commands[command] = handler;
    return handler;
}
