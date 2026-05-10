[BITS 64]

section .text

global gdt_load

gdt_load:
    lgdt [rdi]
.reload_cs:
    push qword 0x08
    lea rax, [rel .reload_segments]
    push rax
    retfq

.reload_segments:
    ret
