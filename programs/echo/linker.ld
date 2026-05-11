ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS
{
    . = 0x400000;
    .text : ALIGN(4096) { *(.asm) *(.text*) }
    .rodata : ALIGN(4096) { *(.rodata*) }
    .data : ALIGN(4096) { *(.data) }
    .bss : ALIGN(4096) { *(.bss) *(COMMON) }
}
