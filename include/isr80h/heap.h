#ifndef ISR80H_HEAP_H
#define ISR80H_HEAP_H

struct interrupt_frame;

void* isr80h_command3_malloc(struct interrupt_frame* frame);
void* isr80h_command4_free(struct interrupt_frame* frame);

#endif /* ISR80H_HEAP_H */