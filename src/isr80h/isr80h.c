#include "isr80h/isr80h.h"
#include "isr80h/io.h"
#include "isr80h/heap.h"
#include "isr80h/process.h"
#include "idt/idt.h"

void isr80h_register_commands()
{
    isr80h_register_command(SYSTEM_COMMAND0_PRINT, isr80h_command0_print);
    isr80h_register_command(SYSTEM_COMMAND1_GETKEY, isr80h_command1_getkey);
    isr80h_register_command(SYSTEM_COMMAND2_PUTCHAR, isr80h_command2_putchar);
    isr80h_register_command(SYSTEM_COMMAND3_MALLOC, isr80h_command3_malloc);
    isr80h_register_command(SYSTEM_COMMAND4_FREE, isr80h_command4_free);
    isr80h_register_command(SYSTEM_COMMAND5_PROCESS_LOAD_START, isr80h_command5_process_load_start);
    isr80h_register_command(SYSTEM_COMMAND6_INVOKE_SYSTEM_COMMAND, isr80h_command6_invoke_system_command);
    isr80h_register_command(SYSTEM_COMMAND7_GET_PROCESS_ARGUMENTS, isr80h_command7_get_process_arguments);
    isr80h_register_command(SYSTEM_COMMAND8_EXIT, isr80h_command8_exit);
}