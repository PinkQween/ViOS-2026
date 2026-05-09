section .asm

global idt_load

global isr80h
extern isr80h_handler

extern interrupt_handler
global interrupt_pointer_table

idt_load:
    push ebp
    mov ebp, esp
    mov ebx, [ebp + 8] 
    lidt [ebx]
    pop ebp
    ret

isr80h:
    push dword 0
    pushad
    
    push esp
    push eax
    call isr80h_handler
    mov dword[tmp_res], eax
    add esp, 8

    popad
    add esp, 4
    mov eax, [tmp_res]
    iret

%macro interrupt_no_error 1
    global int%1
    int%1:
        push dword 0
        pushad

        push esp
        push dword %1
        call interrupt_handler
        add esp, 8

        popad
        add esp, 4
        iret
%endmacro

%macro interrupt_error 1
    global int%1
    int%1:
        pushad

        push esp
        push dword %1
        call interrupt_handler
        add esp, 8

        popad
        add esp, 4
        iret
%endmacro

%assign i 0
%rep 256
%if i = 8 || (i >= 10 && i <= 14) || i = 17 || i = 21
    interrupt_error i
%else
    interrupt_no_error i
%endif
%assign i i+1
%endrep

section .data

tmp_res: dd 0

interrupt_pointer_table:
%assign i 0
%rep 256
    dd int%+i
%assign i i+1
%endrep
