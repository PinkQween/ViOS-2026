#ifndef ISR80H_PROCESS_H
#define ISR80H_PROCESS_H

struct interrupt_frame;

void* isr80h_command5_process_load_start(struct interrupt_frame* frame);
void* isr80h_command6_invoke_system_command(struct interrupt_frame* frame);
void* isr80h_command7_get_process_arguments(struct interrupt_frame* frame);
void* isr80h_command8_exit(struct interrupt_frame* frame);

#endif /* ISR80H_PROCESS_H */