#include "console/console.h"
#include "config.h"
#include "io/io.h"
#include "string/string.h"

#include "graphics/graphics.h"
#include "graphics/text/font.h"
#include "graphics/terminal/terminal.h"

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"

#define COM1_PORT 0x3F8

static bool serial_enabled = false;

extern struct terminal* system_terminal;

static void serial_writechar(char c)
{
    if (!serial_enabled)
        return;

    for (int i = 0; i < 100000; i++)
    {
        if (inb(COM1_PORT + 5) & 0x20)
        {
            outb(COM1_PORT, (uint8_t)c);
            return;
        }
    }

    serial_enabled = false;
}

static void toggle_cursor(bool show)
{
    if (show)
    {
        outb(0x3D4, 0x0A);
        outb(0x3D5, 0x00);
        return;
    }

    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

char choose_colour(char fg, char bg)
{
    return (bg << 4) | (fg & 0x0F);
}

void kernel_console_init()
{
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x03);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xC7);
    outb(COM1_PORT + 4, 0x0B);
    serial_enabled = true;

    toggle_cursor(false);
}

void print_w_color(const char *str, char colour)
{
    if (!system_terminal)
        return;

    size_t len = strnlen(str, MAX_PATH);
    for (size_t i = 0; i < len; i++)
    {
        terminal_write(system_terminal, str[i]);
        serial_writechar(str[i]);
    }
}

void print(const char *str)
{
    print_w_color(str, WHITE);
}

void print_char(char c)
{
    if (system_terminal)
    {
        terminal_write(system_terminal, c);
    }
    serial_writechar(c);
}

void panic(const char *message)
{
    __asm__ __volatile__("cli");
    print_w_color("\n[KERNEL PANIC] ", choose_colour(RED, BLACK));
    print(message);

    for (;;)
    {
        __asm__ __volatile__("hlt");
    }
}

void panic_status(const char *message, status_t status)
{
    char error_message[256];
    snprintf(error_message, sizeof(error_message), "%s: %s (%d)\n", message, status_to_string(status), status);
    panic(error_message);
}

struct framebuffer_pixel vga_foreground_to_pixel_colour(char colour)
{
    static const struct framebuffer_pixel vga_palette[16] =
    {
        {0x00, 0x00, 0x00, 0}, // BLACK
        {0xAA, 0x00, 0x00, 0}, // BLUE
        {0x00, 0xAA, 0x00, 0}, // GREEN
        {0xAA, 0xAA, 0x00, 0}, // CYAN
        {0x00, 0x00, 0xAA, 0}, // RED
        {0xAA, 0x00, 0xAA, 0}, // MAGENTA
        {0x00, 0x55, 0xAA, 0}, // BROWN
        {0xAA, 0xAA, 0xAA, 0}, // LIGHT_GREY
        {0x55, 0x55, 0x55, 0}, // DARK_GREY
        {0xFF, 0x55, 0x55, 0}, // LIGHT_BLUE
        {0x55, 0xFF, 0x55, 0}, // LIGHT_GREEN
        {0xFF, 0xFF, 0x55, 0}, // LIGHT_CYAN
        {0x55, 0x55, 0xFF, 0}, // LIGHT_RED
        {0xFF, 0x55, 0xFF, 0}, // LIGHT_MAGENTA
        {0x55, 0xFF, 0xFF, 0}, // YELLOW
        {0xFF, 0xFF, 0xFF, 0}, // WHITE
    };

    return vga_palette[colour & 0x0F];
}

struct framebuffer_pixel vga_background_to_pixel_colour(char colour)
{
    static const struct framebuffer_pixel vga_palette[16] =
    {
        {0x00, 0x00, 0x00, 0}, // BLACK
        {0xAA, 0x00, 0x00, 0}, // BLUE
        {0x00, 0xAA, 0x00, 0}, // GREEN
        {0xAA, 0xAA, 0x00, 0}, // CYAN
        {0x00, 0x00, 0xAA, 0}, // RED
        {0xAA, 0x00, 0xAA, 0}, // MAGENTA
        {0x00, 0x55, 0xAA, 0}, // BROWN
        {0xAA, 0xAA, 0xAA, 0}, // LIGHT_GREY
        {0x55, 0x55, 0x55, 0}, // DARK_GREY
        {0xFF, 0x55, 0x55, 0}, // LIGHT_BLUE
        {0x55, 0xFF, 0x55, 0}, // LIGHT_GREEN
        {0xFF, 0xFF, 0x55, 0}, // LIGHT_CYAN
        {0x55, 0x55, 0xFF, 0}, // LIGHT_RED
        {0xFF, 0x55, 0xFF, 0}, // LIGHT_MAGENTA
        {0x55, 0xFF, 0xFF, 0}, // YELLOW
        {0xFF, 0xFF, 0xFF, 0}, // WHITE
    };

    return vga_palette[colour >> 4 & 0x0F];
}

void terminal_clear_color_and_reset_cursor(char colour)
{
    if (!system_terminal)
        return;

    system_terminal->font_color =
        vga_foreground_to_pixel_colour(colour);

    struct framebuffer_pixel bg =
        vga_background_to_pixel_colour(colour);

    terminal_clear(system_terminal, bg);
}
