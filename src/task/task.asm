section .asm

global task_return
global restore_general_purpose_registers
global user_registers

task_return:
    mov ebx, [esp + 4] ; struct registers*

    ; Load user data segments before iret to ring 3.
    mov ax, [ebx + 44]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    ; Build iretd frame: SS, ESP, EFLAGS, CS, EIP.
    push dword [ebx + 44]
    push dword [ebx + 40]
    push dword [ebx + 36]
    push dword [ebx + 32]
    push dword [ebx + 28]

    ; Restore GPRs from saved task state.
    mov edi, [ebx]
    mov esi, [ebx + 4]
    mov ebp, [ebx + 8]
    mov edx, [ebx + 16]
    mov ecx, [ebx + 20]
    mov eax, [ebx + 24]
    mov ebx, [ebx + 12]

    iretd

restore_general_purpose_registers:
    push ebp
    mov ebp, esp
    mov ebx, [ebp + 8] ; Load the pointer to the saved registers into ebx
    mov edi, [ebx] ; Load the saved edi value
    mov esi, [ebx + 4] ; Load the saved esi value
    mov ebp, [ebx + 8] ; Load the saved ebp value
    mov edx, [ebx + 16] ; Load the saved edx value
    mov ecx, [ebx + 20] ; Load the saved ecx value
    mov eax, [ebx + 24] ; Load the saved eax value
    mov ebx, [ebx + 12] ; Load the saved ebx value
    pop ebp
    ret

user_registers:
    mov ax, 0x23
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    ret
