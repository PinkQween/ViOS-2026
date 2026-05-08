#ifndef CONFIG_H
#define CONFIG_H

/** Kernel display name string. */
#define KERNEL_NAME "ViOS"

/** GDT selector index for kernel code segment. */
#define KERNEL_CODE_SELECTOR 0x08

/** GDT selector index for kernel data segment. */
#define KERNEL_DATA_SELECTOR 0x10

/** Total interrupt vector slots configured in IDT handling. */
#define TOTAL_INTERRUPTS 256

/** Total bytes reserved for kernel heap region. */
#define HEAP_SIZE_BYTES 100 * 1024 * 1024

/** Heap allocation block granularity in bytes. */
#define HEAP_BLOCK_SIZE 4096

/** Virtual/physical base address of heap memory area. */
#define HEAP_ADDRESS 0x1000000

/** Address of heap allocation table metadata. */
#define HEAP_TABLE_ADDRESS 0x7E00

/** Disk sector size used by low-level storage routines. */
#define SECTOR_SIZE_BYTES 512

/** Maximum supported path length including null terminator. */
#define MAX_PATH_LENGTH 256

/** Maximum number of filesystem drivers that can be registered. */
#define MAX_FILESYSTEMS 12

/** Maximum simultaneously tracked file descriptors. */
#define MAX_FILE_DESCRIPTORS 4096

/** Total GDT segments configured in the GDT table. */
#define TOTAL_GDT_SEGMENTS 6

/** Virtual address where the program is loaded. */
#define PROGRAM_VIRTUAL_ADDRESS 0x400000

/** Virtual address where the program's stack starts. */
#define USER_PROGRAM_STACK_SIZE 0x400000 + 1024 * 16

/** Virtual address where the program's stack starts. */
#define PROGRAM_VIRTUAL_ADDRESS_STACK_START 0x3FF000

/** Virtual address where the program's stack ends. */
#define PROGRAM_VIRTUAL_ADDRESS_STACK_END PROGRAM_VIRTUAL_ADDRESS_STACK_START + USER_PROGRAM_STACK_SIZE

/** GDT selector index for user code segment. */
#define USER_CODE_SEGMENT 0x1B

/** GDT selector index for user data segment. */
#define USER_DATA_SEGMENT 0x23

/** Maximum number of memory allocations a process can track. */
#define MAX_PROGRAM_ALLOCATIONS 1024

/** Maximum number of processes that can be running simultaneously. */
#define MAX_PROCESSES 0x4000

/** Maximum number of commands supported by the isr80h system call handler. */
#define MAX_ISR80H_COMMANDS 1024

#define KEYBOARD_BUFFER_SIZE 1024

#endif /* CONFIG_H */
