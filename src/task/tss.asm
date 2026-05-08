section .asm

global tss_load

tss_load:
    push ebp
    mov ebp, esp
    mov ax, [ebp + 8] ; Load the TSS selector from the stack
    ltr ax              ; Load the TSS selector into the task register
    pop ebp
    ret