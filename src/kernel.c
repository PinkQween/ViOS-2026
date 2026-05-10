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

#include "stdint.h"

struct paging_desc *kernel_paging_desc = 0;
struct tss tss;

struct gdt_table
{
    struct gdt_entry null;
    struct gdt_entry kernel_code;
    struct gdt_entry kernel_data;
    struct gdt_entry user_code;
    struct gdt_entry user_data;
    struct tss_desc_64 tss;
} __attribute__((packed));

static struct gdt_table gdt_real;

void kernel_page()
{
    kernel_registers();
    paging_switch(kernel_paging_desc);
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
    memset(&gdt_real, 0, sizeof(gdt_real));
    gdt_set(&gdt_real.kernel_code, 0, 0, 0x9A, 0x20);
    gdt_set(&gdt_real.kernel_data, 0, 0, 0x92, 0x00);
    gdt_set(&gdt_real.user_code, 0, 0xFFFFF, 0xFA, 0xC0);
    gdt_set(&gdt_real.user_data, 0, 0xFFFFF, 0xF2, 0xC0);
    gdt_set_tss(&gdt_real.tss, &tss, sizeof(struct tss) - 1, 0x89, 0x00);

    struct gdt_ptr gdt_descriptor = {
        .limit = sizeof(gdt_real) - 1,
        .base = (uint64_t)&gdt_real
    };

    gdt_load(&gdt_descriptor);
    kernel_registers();
}

static void kernel_setup_tss()
{
    memset(&tss, 0, sizeof(struct tss));
    tss.rsp0 = 0x60000;
    tss.iomap_base = sizeof(struct tss);

    tss_load(0x28);
}

static void kernel_setup_paging()
{
    kernel_paging_desc = paging_desc_new(PAGING_MAP_LEVEL_4);
    if (!kernel_paging_desc)
    {
        panic_status("Failed to create kernel paging descriptor", STATUS_ERR(ENOMEM));
    }

    paging_map_e820_memory_regions(kernel_paging_desc);
    paging_switch(kernel_paging_desc);
}

static void kernel_load_init_process()
{
    struct process* process = 0;
    status_t status = process_load_switch(ROOT_PROCESS_PATH, &process);
    panic_if_error("Failed to load root process", status);
}

void kernel_main()
{
    kernel_console_init();
    kernel_print_banner();

    print("Initializing GDT...");
    kernel_setup_gdt();

    boot_step("Initializing memory management...");
    kheap_init();

    boot_step("Setting up paging...");
    kernel_setup_paging();

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

    boot_step("Enabling hardware IRQs...");
    idt_unmask_irq(0);
    idt_unmask_irq(1);

    boot_step("Setting up TSS...");
    kernel_setup_tss();

    print_ok();
    kernel_load_init_process();
    task_run_root_task();

    kernel_idle();
}
