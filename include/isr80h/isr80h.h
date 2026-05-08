#ifndef ISR80H_H
#define ISR80H_H

enum SYSTEM_COMMANDS
{
    SYSTEM_COMMAND0_PRINT,
    SYSTEM_COMMAND1_GETKEY,
    SYSTEM_COMMAND2_PUTCHAR,
    SYSTEM_COMMAND3_MALLOC,
    SYSTEM_COMMAND4_FREE,
};

/**
 * ISR 0x80 handler for system calls from user space. This handler is responsible for dispatching system call commands to their registered handlers and managing the transition between user space and kernel space during system call execution.
 */
void isr80h_register_commands();

#endif /* ISR80H_H */
