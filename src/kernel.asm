;--------------------------------------
; ViOS Kernel Stub (32-bit)
;--------------------------------------
[BITS 32]
global _start
global kernel_start
global kernel_idle
global enable_interrupts
global kernel_registers

extern kernel_main
extern kernel_stack_top

CODE_SEG equ 0x08
DATA_SEG equ 0x10

_start:
kernel_start:
    cli                             ; disable interrupts during setup

    ;-----------------------------
    ; Initialize segments
    ;-----------------------------
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ;-----------------------------
    ; Initialize stack
    ;-----------------------------
    mov esp, kernel_stack_top
    mov ebp, esp

    ;-----------------------------
    ; Keep IRQs masked until the kernel installs an IDT and drivers.
    ;-----------------------------
    mov al, 0xFF
    out 0x21, al
    out 0xA1, al

    call kernel_main

kernel_idle:
    sti
    hlt
    cli
    jmp kernel_idle

;-----------------------------
; Enable interrupts wrapper
;-----------------------------
enable_interrupts:
    sti
    ret

;-----------------------------
; Reload data segment registers
;-----------------------------
kernel_registers:
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret
