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
    pushad
    
    push esp
    push eax
    call isr80h_handler
    mov dword[tmp_res], eax
    add esp, 8

    popad
    mov eax, [tmp_res]
    iret

%macro interrupt 1
    global int%1
    int%1:
        pushad

        push esp
        push dword %1
        call interrupt_handler
        add esp, 8

        popad
%if %1 = 8 || (%1 >= 10 && %1 <= 14) || %1 = 17 || %1 = 21
        add esp, 4
%endif
        iret
%endmacro

%assign i 0
%rep 256
    interrupt i
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
