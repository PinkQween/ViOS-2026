[BITS 64]

section .asm

extern isr80h_handler
extern interrupt_handler

global idt_load
global no_interrupt
global disable_interrupts
global isr80h
global isr80h_wrapper
global interrupt_pointer_table

temp_rsp_storage: dq 0x00
%macro pushad_macro 0
    mov qword [rel temp_rsp_storage], rsp
    push rax
    push rcx
    push rdx
    push rbx
    push qword [rel temp_rsp_storage]
    push rbp
    push rsi
    push rdi
%endmacro

%macro popad_macro 0
    pop rdi
    pop rsi
    pop rbp
    pop qword [rel temp_rsp_storage]
    pop rbx
    pop rdx
    pop rcx
    pop rax
    mov rsp, [rel temp_rsp_storage]
%endmacro

disable_interrupts:
    cli
    ret

idt_load:
    mov rbx, rdi
    lidt [rbx]   
    ret

no_interrupt:
    push qword 0
    pushad_macro
    popad_macro
    add rsp, 8
    iretq

%macro interrupt 1
    global int%1
    int%1:
%if %1 != 8 && %1 != 10 && %1 != 11 && %1 != 12 && %1 != 13 && %1 != 14 && %1 != 17 && %1 != 21 && %1 != 29 && %1 != 30
        push qword 0
%endif
        pushad_macro
        mov rdi, %1
        mov rsi, rsp
        call interrupt_handler
        popad_macro
        add rsp, 8
        iretq
%endmacro

%assign i 0
%rep 512
    interrupt i
%assign i i+1
%endrep

isr80h:
isr80h_wrapper:
    push qword 0
    pushad_macro
    
    mov rsi, rsp

    mov rdi, rax
    call isr80h_handler
    mov qword [rel tmp_res], rax
    popad_macro
    add rsp, 8
    mov rax, [rel tmp_res]
    iretq

section .data
tmp_res: dq 0


%macro interrupt_array_entry 1
    dq int%1
%endmacro

interrupt_pointer_table:
%assign i 0
%rep 512
    interrupt_array_entry i
%assign i i+1
%endrep
