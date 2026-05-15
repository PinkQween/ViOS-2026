#include "kernel.h"

#include <stdint.h>

#include "config.h"
#include "console/console.h"
#include "disk/disk.h"
#include "fs/file.h"
#include "gdt/gdt.h"
#include "idt/idt.h"
#include "disk/gpt.h"
#include "isr80h/isr80h.h"
#include "keyboard/keyboard.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "memory/paging/paging.h"
#include "string/string.h"
#include "task/process.h"
#include "task/task.h"
#include "task/tss.h"
#include "graphics/graphics.h"
#include "graphics/text/font.h"
#include "graphics/terminal/terminal.h"
#include "graphics/image/image.h"

extern struct graphics_info default_graphics_info;

/* -------------------------------------------------------------------------- */
/* Globals                                                                    */
/* -------------------------------------------------------------------------- */

struct paging_desc* kernel_paging_desc = NULL;

struct tss tss;

struct terminal* system_terminal = NULL;

/* -------------------------------------------------------------------------- */
/* GDT Layout                                                                 */
/* -------------------------------------------------------------------------- */

struct gdt_table
{
    struct gdt_entry null;
    struct gdt_entry kernel_code;
    struct gdt_entry kernel_data;
    struct gdt_entry reserved1;
    struct gdt_entry reserved2;
    struct gdt_entry user_data;
    struct gdt_entry user_code;
    struct tss_desc_64 tss;
} __attribute__((packed));

static struct gdt_table gdt_real;

/* -------------------------------------------------------------------------- */
/* Paging                                                                     */
/* -------------------------------------------------------------------------- */

struct paging_desc* kernel_desc(void)
{
    return kernel_paging_desc;
}

void kernel_page(void)
{
    kernel_registers();
    paging_switch(kernel_paging_desc);
}

/* -------------------------------------------------------------------------- */
/* Console Helpers                                                            */
/* -------------------------------------------------------------------------- */

static void print_status(
    const char* status,
    uint8_t colour
)
{
    print_w_color("[", colour);
    print_w_color(status, colour);
    print_w_color("] ", colour);
}

static void print_ok(void)
{
    print_status(
        "OK",
        choose_colour(GREEN, BLACK)
    );
}

static void boot_step(const char* message)
{;
    print_ok();
    print(message);
    print("\n");
}

static void panic_if_error(
    const char* message,
    status_t status
)
{
    if (status_is_error(status))
    {
        panic_status(message, status);
    }
}

static void kernel_print_banner(void)
{
    print("Welcome to ");
    print(KERNEL_NAME);
    print("!\n");
}

/* -------------------------------------------------------------------------- */
/* GDT / TSS                                                                  */
/* -------------------------------------------------------------------------- */

static void kernel_setup_gdt(void)
{
    memset(&gdt_real, 0, sizeof(gdt_real));

    gdt_set(
        &gdt_real.kernel_code,
        0,
        0xFFFFF,
        0x9A,
        0x20
    );

    gdt_set(
        &gdt_real.kernel_data,
        0,
        0xFFFFF,
        0x92,
        0x00
    );

    gdt_set(
        &gdt_real.user_data,
        0,
        0xFFFFF,
        0xF2,
        0x00
    );

    gdt_set(
        &gdt_real.user_code,
        0,
        0xFFFFF,
        0xFA,
        0x20
    );

    gdt_set_tss(
        &gdt_real.tss,
        &tss,
        sizeof(struct tss) - 1,
        0x89,
        0x00
    );

    struct gdt_ptr gdt_descriptor =
    {
        .limit = sizeof(gdt_real) - 1,
        .base = (uint64_t)&gdt_real
    };

    gdt_load(&gdt_descriptor);

    kernel_registers();
}

static void kernel_setup_tss(void)
{
    memset(&tss, 0, sizeof(struct tss));

    tss.rsp0 = 0x60000;
    tss.iomap_base = sizeof(struct tss);

    tss_load(
        KERNEL_LONG_MODE_TSS_GDT_INDEX << 3
    );
}

/* -------------------------------------------------------------------------- */
/* Memory Management                                                          */
/* -------------------------------------------------------------------------- */

static void kernel_setup_paging(void)
{
    kernel_paging_desc =
        paging_desc_new(PAGING_MAP_LEVEL_4);

    if (!kernel_paging_desc)
    {
        panic_status(
            "Failed to create kernel paging descriptor",
            STATUS_ERR(ENOMEM)
        );
    }

    paging_map_e820_memory_regions(
        kernel_paging_desc
    );

    paging_switch(kernel_paging_desc);
}

/* -------------------------------------------------------------------------- */
/* Process Initialization                                                     */
/* -------------------------------------------------------------------------- */

static void kernel_load_init_process(void)
{
    struct process* process = NULL;

    status_t status =
        process_load(
            ROOT_PROCESS_PATH,
            &process
        );

    panic_if_error(
        "Failed to load root process",
        status
    );
}

/* -------------------------------------------------------------------------- */
/* Kernel Entry                                                               */
/* -------------------------------------------------------------------------- */

void kernel_main(void)
{
    kernel_console_init();
    kernel_print_banner();

    kernel_setup_gdt();
    boot_step("Initializing GDT");

    kheap_init();
    boot_step("Initializing heap");

    kernel_setup_paging();
    boot_step("Setting up paging");

    kheap_post_paging();
    boot_step("Initializing post-paging heap");

    graphics_setup(&default_graphics_info);

    fs_init();
    boot_step("Initializing filesystem");

    panic_if_error(
        "Failed to initialize disks",
        disk_search_and_init()
    );
    boot_step("Initializing disks");

    gpt_init();
    boot_step("Initializing GPT");

    font_system_init();
    boot_step("Initializing font system");

    terminal_system_setup();
    {
        struct graphics_info* screen_info = graphics_screen_info();
        struct font* font = font_get_system_font();
        if (font && screen_info)
        {
            struct framebuffer_pixel white = {255, 255, 255, 255};
            struct framebuffer_pixel black = {0, 0, 0, 0};
            
            graphics_draw_rect(screen_info, 0, 0, screen_info->width, screen_info->height, black);
            graphics_redraw_graphics_to_screen(screen_info, 0, 0, screen_info->width, screen_info->height);
            
            system_terminal = terminal_create(
                screen_info, 0, 0,
                screen_info->width, screen_info->height,
                font, white,
                TERMINAL_FLAG_BACKSPACE_ALLOWED
            );
        }
    }

    idt_init();
    boot_step("Initializing IDT");

    isr80h_register_commands();
    boot_step("Registering ISR80H commands");

    panic_if_error(
        "Failed to initialize keyboard",
        keyboard_init()
    );
    boot_step("Initializing keyboard");

    kernel_setup_tss();
    boot_step("Initializing TSS");

    kernel_load_init_process();
    boot_step("Loading init process");

    terminal_clear_color_and_reset_cursor(choose_colour(WHITE, BLACK));

    idt_unmask_irq(0);
    idt_unmask_irq(1);

    task_run_root_task();

    kernel_idle();
}
