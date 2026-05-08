#include "kernel.h"
#include "config.h"
#include "console/console.h"
#include "disk/disk.h"
#include "fs/file.h"
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "isr80h/isr80h.h"
#include "keyboard/keyboard.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "string/string.h"
#include "task/task.h"
#include "task/tss.h"

#include <stdint.h>

struct paging_4gb_chunk *kernel_chunk = 0;
struct tss tss;

struct gdt_entry gdt_real[TOTAL_GDT_SEGMENTS];
struct gdt_structured gdt_structured_real[TOTAL_GDT_SEGMENTS] = {
    { .base = 0x00000000, .limit = 0x00000000, .type = 0x00 },
    { .base = 0x00000000, .limit = 0xFFFFFFFF, .type = 0x9A },
    { .base = 0x00000000, .limit = 0xFFFFFFFF, .type = 0x92 },
    { .base = 0x00000000, .limit = 0xFFFFFFFF, .type = 0xFA },
    { .base = 0x00000000, .limit = 0xFFFFFFFF, .type = 0xF2 },
    { .base = (uint32_t)&tss, .limit = (uint32_t)(sizeof(struct tss) - 1), .type = 0xE9 }
};

void kernel_page()
{
    kernel_registers();
    paging_switch(kernel_chunk);
}

static void print_status_w_colour(const char* message, char colour)
{
    print_w_color("[", colour);
    print_w_color(message, colour);
    print_w_color("]\n", colour);
}

static void print_ok()
{
    print_status_w_colour("OK", choose_colour(GREEN, BLACK));
}

static void boot_step(const char* message)
{
    print_ok();
    print(message);
}

static void panic_if_error(const char* message, status_t status)
{
    if (status_is_error(status))
    {
        panic_status(message, status);
    }
}

static void kernel_print_banner()
{
    print("Welcome to ");
    print(KERNEL_NAME);
    print("!\n");
}

static void kernel_setup_gdt()
{
    memset(gdt_real, 0, sizeof(gdt_real));
    gdt_structured_to_gdt(gdt_real, gdt_structured_real, TOTAL_GDT_SEGMENTS);
    gdt_load(gdt_real, sizeof(gdt_real));
}

static void kernel_setup_tss()
{
    memset(&tss, 0, sizeof(struct tss));
    tss.esp0 = 0x60000;
    tss.ss0 = KERNEL_DATA_SELECTOR;
    tss.iomap_base = sizeof(struct tss);

    tss_load(0x28);
}

static void kernel_setup_paging()
{
    kernel_chunk = paging_new_4gb(PAGING_IS_WRITEABLE | PAGING_ACCESSIBLE_FROM_ALL | PAGING_IS_PRESENT);
    if (!kernel_chunk)
    {
        panic_status("Failed to create kernel paging chunk", STATUS_ERR(ENOMEM));
    }

    paging_switch(kernel_chunk);
}

static void kernel_load_init_process()
{
    struct process* process = 0;
    status_t status = process_load_switch("0:/hello.elf", &process);
    panic_if_error("Failed to load process", status);
}

void kernel_main()
{
    kernel_console_init();
    kernel_print_banner();

    print("Initializing GDT...");
    kernel_setup_gdt();

    boot_step("Initializing memory management...");
    kheap_init();

    boot_step("Initializing filesystem...");
    fs_init();

    boot_step("Initializing disks...");
    panic_if_error("Failed to initialize disks", disk_search_and_init());

    boot_step("Initializing IDT...");
    idt_init();

    boot_step("Initializing ISR80H...");
    isr80h_register_commands();

    boot_step("Initializing keyboard...");
    panic_if_error("Failed to initialize keyboard", keyboard_init());

    boot_step("Setting up TSS...");
    kernel_setup_tss();

    boot_step("Setting up paging...");
    kernel_setup_paging();

    boot_step("Enabling paging...");
    enable_paging();

    print_ok();
    kernel_load_init_process();
    task_run_root_task();

    for (;;)
    {
        __asm__ __volatile__("hlt");
    }
}
