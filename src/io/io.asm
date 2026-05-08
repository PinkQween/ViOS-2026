section .asm

; =========================================================
; STRING INPUT (port -> memory)
; =========================================================

global insb
insb:
    push ebp
    mov ebp, esp
    push edi
    push ecx
    push edx

    mov dx,  [ebp + 8]     ; port
    mov edi, [ebp + 12]    ; buffer
    mov ecx, [ebp + 16]    ; count

    cld
    rep insb

    pop edx
    pop ecx
    pop edi
    pop ebp
    ret


global insw
insw:
    push ebp
    mov ebp, esp
    push edi
    push ecx
    push edx

    mov dx,  [ebp + 8]
    mov edi, [ebp + 12]
    mov ecx, [ebp + 16]

    cld
    rep insw

    pop edx
    pop ecx
    pop edi
    pop ebp
    ret


global insd
insd:
    push ebp
    mov ebp, esp
    push edi
    push ecx
    push edx

    mov dx,  [ebp + 8]
    mov edi, [ebp + 12]
    mov ecx, [ebp + 16]

    cld
    rep insd

    pop edx
    pop ecx
    pop edi
    pop ebp
    ret


; =========================================================
; STRING OUTPUT (memory -> port)
; =========================================================

global outsb
outsb:
    push ebp
    mov ebp, esp
    push esi
    push ecx
    push edx

    mov dx,  [ebp + 8]
    mov esi, [ebp + 12]
    mov ecx, [ebp + 16]

    cld
    rep outsb

    pop edx
    pop ecx
    pop esi
    pop ebp
    ret


global outsw
outsw:
    push ebp
    mov ebp, esp
    push esi
    push ecx
    push edx

    mov dx,  [ebp + 8]
    mov esi, [ebp + 12]
    mov ecx, [ebp + 16]

    cld
    rep outsw

    pop edx
    pop ecx
    pop esi
    pop ebp
    ret


global outsd
outsd:
    push ebp
    mov ebp, esp
    push esi
    push ecx
    push edx

    mov dx,  [ebp + 8]
    mov esi, [ebp + 12]
    mov ecx, [ebp + 16]

    cld
    rep outsd

    pop edx
    pop ecx
    pop esi
    pop ebp
    ret


; =========================================================
; SINGLE PORT I/O
; =========================================================

global inb
inb:
    push ebp
    mov ebp, esp

    mov dx, [ebp + 8]
    in al, dx
    movzx eax, al

    pop ebp
    ret


global inw
inw:
    push ebp
    mov ebp, esp

    mov dx, [ebp + 8]
    in ax, dx
    movzx eax, ax

    pop ebp
    ret


global ind
ind:
    push ebp
    mov ebp, esp

    mov dx, [ebp + 8]
    in eax, dx

    pop ebp
    ret


global outb
outb:
    push ebp
    mov ebp, esp

    mov dx, [ebp + 8]
    mov al, [ebp + 12]
    out dx, al

    pop ebp
    ret


global outw
outw:
    push ebp
    mov ebp, esp

    mov dx, [ebp + 8]
    mov ax, [ebp + 12]
    out dx, ax

    pop ebp
    ret


global outd
outd:
    push ebp
    mov ebp, esp

    mov dx, [ebp + 8]
    mov eax, [ebp + 12]
    out dx, eax

    pop ebp
    ret