[BITS 32]
global _start
global kernel_start
global kernel_bios_entry
global kernel_idle
global enable_interrupts
global kernel_registers
global kernel_uefi_entry
global kernel_long_mode_entry
global default_graphics_info
global gdt

extern kernel_main
extern kernel_stack_top

CODE_SEG            equ 0x08
DATA_SEG            equ 0x10
LONG_MODE_CODE_SEG  equ 0x18
LONG_MODE_DATA_SEG  equ 0x10

align 16
_start:
kernel_start:
kernel_bios_entry:
    cli                         ; disable interrupts during setup

    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    mov esp, kernel_stack_top
    mov ebp, esp

    lgdt [gdt_descriptor]

    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    mov eax, PML4_Table
    mov cr3, eax

    mov ecx, 0xC0000080
    rdmsr
    or eax, 0x100
    wrmsr

    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    jmp LONG_MODE_CODE_SEG:kernel_long_mode_entry

[BITS 64]
kernel_registers:
    mov ax, LONG_MODE_DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    ret

align 4096
kernel_uefi_entry:
    lgdt [gdt_descriptor]

kernel_long_mode_entry:
    cli
    call kernel_registers

    mov rsp, kernel_stack_top
    mov rbp, rsp

    mov [default_graphics_info + 0], rdi

    mov [default_graphics_info + 8], edx
    mov [default_graphics_info + 12], ecx
    mov [default_graphics_info + 16], esi

    mov al, 0x11
    out 0x20, al

    mov al, 0x20
    out 0x21, al

    mov al, 0x04
    out 0x21, al

    mov al, 0x01
    out 0x21, al

    mov al, 0x11
    out 0xA0, al

    mov al, 0x28
    out 0xA1, al

    mov al, 0x02
    out 0xA1, al

    mov al, 0x01
    out 0xA1, al

    mov al, 0xFB
    out 0x21, al

    mov al, 0xFF
    out 0xA1, al

    mov al, 0x20
    out 0x20, al
    out 0xA0, al

    call kernel_main
    jmp kernel_idle

kernel_idle:
    sti
.idle_loop:
    hlt
    jmp .idle_loop

enable_interrupts:
    sti
    ret

align 8
gdt:
    dq 0x0000000000000000

    dw 0xffff
    dw 0x0000
    db 0x00
    db 0x9A
    db 11001111b
    db 0x00

    dw 0xffff
    dw 0x0000
    db 0x00
    db 0x92
    db 11001111b
    db 0x00

    dw 0x0000
    dw 0x0000
    db 0x00
    db 0x9A
    db 0x20
    db 0x00

    dw 0x0000
    dw 0x0000
    db 0x00
    db 0x92
    db 0x00
    db 0x00

    dw 0x0000
    dw 0x0000
    db 0x00
    db 0xFA
    db 0x20
    db 0x00

    dw 0x0000
    dw 0x0000
    db 0x00
    db 0xF2
    db 0x00
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt - 1
    dq gdt

align 4096
PML4_Table:
    dq PDPT_Table + 0x03
    times 511 dq 0

align 4096
PDPT_Table:
    dq PD_Table + 0x03
    times 511 dq 0

align 4096
PD_Table:
    %assign page_addr 0
    %assign page_flags 0x83
    %assign page_index 0
    %rep 100
    dq page_addr + page_flags
    %assign page_addr page_addr + 0x200000
    %assign page_index page_index + 1
    %endrep
    times 412 dq 0

align 8
default_graphics_info:
    dq 0
    dd 0
    dd 0
    dd 0
    dd 0
    dq 0
    dd 0
    dd 0
    dd 0
    dd 0
    dd 0
    dd 0
    dq 0
    dq 0
    dd 0
    dd 0
    dd 0
    dd 0
    dq 0
    dq 0