section .asm

global vios_print:function
global vios_getkey:function
global vios_putchar:function
global vios_malloc:function
global vios_free:function

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