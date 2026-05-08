global _start
extern main

section .asm exec

_start:
    call main

    ; If main returns, loop indefinitely. until we implement process termination and cleanup.
    jmp $