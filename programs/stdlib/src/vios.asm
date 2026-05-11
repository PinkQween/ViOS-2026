section .asm

global vios_print:function
global vios_getkey:function
global vios_putchar:function
global vios_malloc:function
global vios_free:function
global vios_process_load_start:function
global vios_invoke_system_command:function
global vios_process_get_arguments:function
global vios_exit:function

vios_print:
    push qword rdi
    mov rax, 0 ; SYSTEM_COMMAND0_PRINT
    int 0x80
    add rsp, 8
    ret

vios_getkey:
    mov rax, 1 ; SYSTEM_COMMAND1_GETKEY
    int 0x80
    ret

vios_putchar:
    mov rax, 2 ; SYSTEM_COMMAND2_PUTCHAR
    push qword rdi
    int 0x80
    add rsp, 8
    ret

vios_malloc:
    mov rax, 3 ; SYSTEM_COMMAND3_MALLOC
    push qword rdi
    int 0x80
    add rsp, 8
    ret

vios_free:
    mov rax, 4 ; SYSTEM_COMMAND4_FREE
    push qword rdi
    int 0x80
    add rsp, 8
    ret

vios_process_load_start:
    mov rax, 5 ; SYSTEM_COMMAND5_PROCESS_LOAD_START
    push qword rdi
    int 0x80
    add rsp, 8
    ret

vios_invoke_system_command:
    mov rax, 6 ; SYSTEM_COMMAND6_INVOKE_SYSTEM_COMMAND
    push qword rdi
    int 0x80
    add rsp, 8
    ret

vios_process_get_arguments:
    mov rax, 7 ; SYSTEM_COMMAND7_PROCESS_GET_ARGUMENTS
    push rdi ; argument: pointer to process_arguments struct
    int 0x80
    add rsp, 8
    ret

vios_exit:
    mov rax, 8 ; SYSTEM_COMMAND8_EXIT
    int 0x80
    ret