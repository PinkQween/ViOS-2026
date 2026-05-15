#ifndef ISR80H_FILE_H
#define ISR80H_FILE_H

struct interrupt_frame;

void* isr80h_command9_fopen(struct interrupt_frame* frame);

#endif /* ISR80H_FILE_H */