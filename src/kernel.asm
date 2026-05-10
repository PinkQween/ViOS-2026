;=============================================
; ViOS Kernel Stub (32-bit -> 64-bit)
;=============================================
[BITS 32]
global _start
global kernel_start
global kernel_idle
global enable_interrupts
global kernel_registers

extern kernel_main
extern kernel_stack_top

;---------------------------------------------
; Segment Selectors (GDT entries)
;---------------------------------------------
CODE_SEG            equ 0x08
DATA_SEG            equ 0x10
LONG_MODE_CODE_SEG  equ 0x18
LONG_MODE_DATA_SEG  equ 0x10

;---------------------------------------------
; Entry point (32-bit protected mode)
;---------------------------------------------
_start:
kernel_start:
    cli                         ; disable interrupts during setup

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
    ; Load the GDT
    ;-----------------------------
    lgdt [gdt_descriptor]

    ;-----------------------------
    ; Enable PAE in CR4
    ;-----------------------------
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ;-----------------------------
    ; Setup paging tables
    ;-----------------------------
    mov eax, PML4_Table
    mov cr3, eax

    ;-----------------------------
    ; Enable long mode in IA32_EFER
    ;-----------------------------
    mov ecx, 0xC0000080       ; IA32_EFER MSR
    rdmsr
    or eax, 0x100             ; set LME bit
    wrmsr

    ;-----------------------------
    ; Enable paging in CR0
    ;-----------------------------
    mov eax, cr0
    or eax, 1 << 31           ; set PG bit
    mov cr0, eax

    ;-----------------------------
    ; Jump to 64-bit code segment
    ;-----------------------------
    jmp LONG_MODE_CODE_SEG:long_mode_entry

[BITS 64]
;=============================================
; 64-bit Kernel Code
;=============================================

;---------------------------------------------
; Load 64-bit data segments
;---------------------------------------------
kernel_registers:
    mov ax, LONG_MODE_DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret

;=============================================
; 64-bit long mode entry point
;=============================================
long_mode_entry:
    ;-----------------------------
    ; Load 64-bit data segments
    ;-----------------------------
    call kernel_registers

    ;-----------------------------
    ; Initialize 64-bit stack
    ;-----------------------------
    mov rsp, 0x00200000        ; safe stack location
    mov rbp, rsp

    ;-----------------------------
    ; Jump to kernel main
    ;-----------------------------
    call kernel_main

    ;-----------------------------
    ; Enter idle loop after kernel_main
    ;-----------------------------
    jmp kernel_idle

;=============================================
; Kernel idle loop
;=============================================
kernel_idle:
    sti
.idle_loop:
    hlt
    jmp .idle_loop

;=============================================
; Enable interrupts wrapper
;=============================================
enable_interrupts:
    sti
    ret

;=============================================
; Global Descriptor Table (GDT)
;=============================================
align 8
gdt:
    dq 0x0000000000000000                ; Null descriptor

    ; 32-bit code segment
    dw 0xffff
    dw 0x0000
    db 0x00
    db 0x9A
    db 11001111b
    db 0x00

    ; 32-bit data segment
    dw 0xffff
    dw 0x0000
    db 0x00
    db 0x92
    db 11001111b
    db 0x00

    ; 64-bit code segment
    dw 0x0000
    dw 0x0000
    db 0x00
    db 0x9A
    db 0x20
    db 0x00

    ; 64-bit data segment
    dw 0x0000
    dw 0x0000
    db 0x00
    db 0x92
    db 0x00
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt - 1
    dd gdt

;=============================================
; Page Tables (64-bit long mode)
;=============================================
%define PAGE_FLAGS 0x03
%define PAGE_INCREMENT 0x1000

align 4096
PML4_Table:
    dq PDPT_Table + PAGE_FLAGS         ; Present | RW
    times 511 dq 0

align 4096
PDPT_Table:
    dq PD_Table + PAGE_FLAGS           ; Present | RW
    times 511 dq 0

align 4096
PD_Table:
    %assign table_offset 0
    %rep 100
    dq PT_Table + table_offset + PAGE_FLAGS
    %assign table_offset table_offset + 0x1000
    %endrep
    times 412 dq 0

align 4096
PT_Table:
    %assign addr 0x00000000
    %rep 512*100
        dq addr + PAGE_FLAGS
        %assign addr addr + PAGE_INCREMENT
    %endrep
