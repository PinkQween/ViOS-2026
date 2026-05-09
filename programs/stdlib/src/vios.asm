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
    push ebp
    mov ebp, esp
    push ebx
    mov eax, 0 ; SYSTEM_COMMAND0_PRINT
    mov ebx, [ebp+8] ; argument: string
    push ebx
    int 0x80
    add esp, 4
    pop ebx
    pop ebp
    ret

vios_getkey:
    push ebp
    mov ebp, esp
    mov eax, 1 ; SYSTEM_COMMAND1_GETKEY
    int 0x80
    pop ebp
    ret

vios_putchar:
    push ebp
    mov ebp, esp
    push ebx
    mov eax, 2 ; SYSTEM_COMMAND2_PUTCHAR
    mov ebx, [ebp+8] ; argument: char
    push ebx
    int 0x80
    add esp, 4
    pop ebx
    pop ebp
    ret

vios_malloc:
    push ebp
    mov ebp, esp
    push ebx
    mov eax, 3 ; SYSTEM_COMMAND3_MALLOC
    mov ebx, [ebp+8] ; argument: size
    push ebx
    int 0x80
    add esp, 4
    pop ebx
    pop ebp
    ret

vios_free:
    push ebp
    mov ebp, esp
    push ebx
    mov eax, 4 ; SYSTEM_COMMAND4_FREE
    mov ebx, [ebp+8] ; argument: pointer
    push ebx
    int 0x80
    add esp, 4
    pop ebx
    pop ebp
    ret

vios_process_load_start:
    push ebp
    mov ebp, esp
    push ebx
    mov eax, 5 ; SYSTEM_COMMAND5_PROCESS_LOAD_START
    mov ebx, [ebp+8] ; argument: filename
    push ebx
    int 0x80
    add esp, 4
    pop ebx
    pop ebp
    ret

vios_invoke_system_command:
    push ebp
    mov ebp, esp
    push ebx
    mov eax, 6 ; SYSTEM_COMMAND6_INVOKE_SYSTEM_COMMAND
    mov ebx, [ebp+8] ; argument: pointer to command_argument struct
    push ebx
    int 0x80
    add esp, 4
    pop ebx
    pop ebp
    ret

vios_process_get_arguments:
    push ebp
    mov ebp, esp
    mov eax, 7 ; SYSTEM_COMMAND7_PROCESS_GET_ARGUMENTS
    push dword[ebp+8] ; argument: pointer to process_arguments struct
    int 0x80
    add esp, 4
    pop ebp
    ret

vios_exit:
    push ebp
    mov ebp, esp
    mov eax, 8 ; SYSTEM_COMMAND8_EXIT
    push dword[ebp+8] ; argument: exit code
    int 0x80
    add esp, 4
    pop ebp
    ret