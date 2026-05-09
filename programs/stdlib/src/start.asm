global _start
extern c_start

section .asm

_start:
    call c_start
    ret