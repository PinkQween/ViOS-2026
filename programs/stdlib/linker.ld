OUTPUT_FORMAT(elf32-i386)
SEARCH_DIR(.)

SECTIONS
{
    . = 0;
    .text : { *(.asm) *(.text) }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(.bss) }
}
