#ifndef ISR80H_H
#define ISR80H_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file isr80h.h
 * @brief ISR 0x80 system call dispatcher interface.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

enum SYSTEM_COMMANDS
{
    SYSTEM_COMMAND0_PRINT,
    SYSTEM_COMMAND1_GETKEY,
    SYSTEM_COMMAND2_PUTCHAR,
    SYSTEM_COMMAND3_MALLOC,
    SYSTEM_COMMAND4_FREE,
    SYSTEM_COMMAND5_PROCESS_LOAD_START,
    SYSTEM_COMMAND6_INVOKE_SYSTEM_COMMAND,
    SYSTEM_COMMAND7_GET_PROCESS_ARGUMENTS,
    SYSTEM_COMMAND8_EXIT,
    SYSTEM_COMMAND9_FOPEN
};

/**
 * ISR 0x80 handler for system calls from user space. This handler is responsible for dispatching system call commands to their registered handlers and managing the transition between user space and kernel space during system call execution.
 */
void isr80h_register_commands();

#endif /* ISR80H_H */
