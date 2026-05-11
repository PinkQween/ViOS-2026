OUTPUT_FORMAT(elf64-x86-64)
SEARCH_DIR(.)

SECTIONS
{
    . = 0;
    .text : { *(.asm) *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}
