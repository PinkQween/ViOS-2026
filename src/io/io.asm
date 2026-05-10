; io.asm - x86_64 I/O routines
; ---------------------------------------------------------
; Implements port I/O functions for 64-bit OS kernel
; ---------------------------------------------------------

[BITS 64]

; =========================================================
; STRING INPUT (port -> memory)
; C functions: insb, insw, insd
; =========================================================

global insb
insb:
    ; C args: rdi=port, rsi=buffer, rdx=count
    mov dx, di      ; port -> dx
    mov rdi, rsi    ; buffer -> rdi
    mov rcx, rdx    ; count -> rcx
    cld
    rep insb
    ret

global insw
insw:
    mov dx, di
    mov rdi, rsi
    mov rcx, rdx
    cld
    rep insw
    ret

global insd
insd:
    mov dx, di
    mov rdi, rsi
    mov rcx, rdx
    cld
    rep insd
    ret

; =========================================================
; STRING OUTPUT (memory -> port)
; C functions: outsb, outsw, outsd
; =========================================================

global outsb
outsb:
    mov dx, di      ; port
    mov rsi, rsi    ; buffer already in rsi
    mov rcx, rdx    ; count
    cld
    rep outsb
    ret

global outsw
outsw:
    mov dx, di
    mov rsi, rsi
    mov rcx, rdx
    cld
    rep outsw
    ret

global outsd
outsd:
    mov dx, di
    mov rsi, rsi
    mov rcx, rdx
    cld
    rep outsd
    ret

; =========================================================
; SINGLE PORT I/O
; C functions: inb, inw, ind, outb, outw, outd
; =========================================================

global inb
inb:
    mov dx, di      ; port
    in al, dx
    movzx eax, al
    ret

global inw
inw:
    mov dx, di
    in ax, dx
    movzx eax, ax
    ret

global ind
ind:
    mov dx, di
    in eax, dx
    ret

global outb
outb:
    mov dx, di      ; port
    mov al, sil     ; value in C: uint8_t = lower 8 bits of rsi (second arg)
    out dx, al
    ret

global outw
outw:
    mov dx, di
    mov ax, si      ; value in C: uint16_t = lower 16 bits of rsi
    out dx, ax
    ret

global outd
outd:
    mov dx, di
    mov eax, esi    ; value in C: uint32_t = lower 32 bits of rsi
    out dx, eax
    ret