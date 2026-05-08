[BITS 32]

section .text

global _start
global division_by_zero
global kernel_registers

extern kernel_main
extern enable_interrupts

CODE_SEG equ 0x08
DATA_SEG equ 0x10

_start:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov ebp, 0x00200000
    mov esp, ebp

    cli

    in al, 0x92
    or al, 0x02
    out 0x92, al

    ; Remap PIC IRQs from 0x08-0x0F/0x70-0x77 to 0x20-0x2F
    mov al, 00010001b
    out 0x20, al
    out 0xA0, al

    mov al, 0x20
    out 0x21, al
    mov al, 0x28
    out 0xA1, al

    mov al, 00000100b
    out 0x21, al
    mov al, 00000010b
    out 0xA1, al

    mov al, 00000001b
    out 0x21, al
    out 0xA1, al

    ; Unmask all IRQ lines.
    mov al, 0x00
    out 0x21, al
    out 0xA1, al

    call kernel_main

    jmp $

enable_interrupts:
    sti
    ret

kernel_registers:
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret

times 512-($-$$) db 0
