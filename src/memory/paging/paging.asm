[BITS 64]

section .asm

global paging_load_directory
global paging_invalidate_tlb_entry

paging_load_directory:
    mov rax, rdi
    mov cr3, rax
    ret

paging_invalidate_tlb_entry:
    invlpg [rdi]
    ret