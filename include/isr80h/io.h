#ifndef ISR80H_IO_H
#define ISR80H_IO_H

struct interrupt_frame;

void* isr80h_command0_print(struct interrupt_frame* frame);
void* isr80h_command1_getkey(struct interrupt_frame* frame);
void* isr80h_command2_putchar(struct interrupt_frame* frame);

#endif /* ISR80H_IO_H */
