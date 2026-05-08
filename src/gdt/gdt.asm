section .asm

global gdt_load

gdt_load:
    push ebp
    mov ebp, esp

    mov eax, [ebp + 8] ; Load the base address of the GDT into eax
    mov edx, [ebp + 12] ; Load the total size of the GDT into edx

    dec edx ; LGDT expects limit = size - 1

    mov [gdt_descriptor], dx ; Set the limit (size of GDT - 1)
    mov [gdt_descriptor + 2], eax ; Set the base address of the GDT

    lgdt [gdt_descriptor] ; Load the GDT using the lgdt instruction

    pop ebp
    ret

section .data
gdt_descriptor:
    dw 0 ; Limit (size of GDT - 1)
    dd 0 ; Base address of GDT