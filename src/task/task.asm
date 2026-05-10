[BITS 64]

section .asm

global task_return
global restore_general_purpose_registers
global user_registers

task_return:
    push qword [rdi+88]
    push qword [rdi+80]
    mov rax, [rdi+72]
    or rax, 0x200
    push rax

    push qword [rdi+64]
    push qword [rdi+56]
    call restore_general_purpose_registers

    iretq

restore_general_purpose_registers:
    mov rsi, [rdi+8]
    mov rbp, [rdi+16]
    mov rbx, [rdi+24]
    mov rdx, [rdi+32]
    mov rcx, [rdi+40]
    mov rax, [rdi+48]
    mov rdi, [rdi]
    ret


user_registers:
    mov ax, 0x2B
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret
