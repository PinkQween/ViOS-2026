#include "console/console.h"
#include "config.h"
#include "io/io.h"
#include "string/string.h"

#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"

#define COM1_PORT 0x3F8

static volatile uint16_t *video_mem = 0;
static uint16_t terminal_row = 0;
static uint16_t terminal_column = 0;
static bool serial_enabled = false;

static void serial_init(void)
{
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x80);
    outb(COM1_PORT + 0, 0x03);
    outb(COM1_PORT + 1, 0x00);
    outb(COM1_PORT + 3, 0x03);
    outb(COM1_PORT + 2, 0xC7);
    outb(COM1_PORT + 4, 0x0B);
    serial_enabled = true;
}

static void serial_writechar(char c)
{
    if (!serial_enabled)
    {
        return;
    }

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

static uint16_t terminal_make_char(char c, char colour)
{
    return ((uint16_t)(uint8_t)colour << 8) | (uint8_t)c;
}

static void terminal_putchar(int x, int y, char c, char colour)
{
    if (x < 0 || x >= VGA_WIDTH || y < 0 || y >= VGA_HEIGHT)
    {
        return;
    }

    video_mem[(y * VGA_WIDTH) + x] = terminal_make_char(c, colour);
}

static void terminal_scroll(void)
{
    for (int y = 1; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            uint16_t cell = video_mem[y * VGA_WIDTH + x];
            terminal_putchar(x, y - 1, cell & 0xFF, cell >> 8);
        }
    }

    for (int x = 0; x < VGA_WIDTH; x++)
    {
        terminal_putchar(x, VGA_HEIGHT - 1, ' ', WHITE);
    }

    terminal_row = VGA_HEIGHT - 1;
}

void terminal_backspace(void)
{
    if (terminal_column == 0 && terminal_row == 0)
    {
        return;
    }

    if (terminal_column == 0)
    {
        terminal_row--;
        terminal_column = VGA_WIDTH - 1;
    }
    else
    {
        terminal_column--;
    }

    terminal_putchar(terminal_column, terminal_row, ' ', WHITE);
}

static void terminal_writechar(char c, char colour);

void terminal_tab(void)
{
    int spaces_to_next_tab_stop = 4 - (terminal_column % 4);
    for (int i = 0; i < spaces_to_next_tab_stop; i++)
    {
        terminal_writechar(' ', WHITE);
    }
}

static void terminal_writechar(char c, char colour)
{
    serial_writechar(c);

    if (c == '\n')
    {
        terminal_column = 0;
        terminal_row++;
    }
    else if (c == 0x08) // Backspace
    {
        terminal_backspace();
    }
    else if (c == '\t')
    {
        terminal_tab();
    }
    else
    {
        terminal_putchar(terminal_column, terminal_row, c, colour);
        terminal_column++;
    }

    if (terminal_column >= VGA_WIDTH)
    {
        terminal_column = 0;
        terminal_row++;
    }

    if (terminal_row >= VGA_HEIGHT)
    {
        terminal_scroll();
    }
}

static void terminal_init(void)
{
    video_mem = (volatile uint16_t *)0xB8000;
    terminal_row = 0;
    terminal_column = 0;

    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(x, y, ' ', WHITE);
        }
    }
}

void kernel_console_init()
{
    serial_init();
    toggle_cursor(false);
    terminal_init();
}

void print_w_color(const char *str, char colour)
{
    size_t len = strnlen(str, MAX_PATH);
    for (size_t i = 0; i < len; i++)
    {
        if (str[i] == ' ')
        {
            terminal_writechar(' ', colour);
            continue;
        }

        size_t word_len = 0;
        while (i + word_len < len && str[i + word_len] != ' ')
        {
            word_len++;
        }

        if (terminal_column + word_len >= VGA_WIDTH)
        {
            terminal_writechar('\n', colour);
        }

        for (size_t j = 0; j < word_len; j++)
        {
            terminal_writechar(str[i + j], colour);
        }

        i += word_len - 1;
    }
}

void print(const char *str)
{
    print_w_color(str, WHITE);
}

void print_char(char c)
{
    terminal_writechar(c, WHITE);
}

void panic(const char *message)
{
    __asm__ __volatile__("cli");
    print_w_color("KERNEL PANIC: ", choose_colour(RED, BLACK));
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

void terminal_clear_color_and_reset_cursor(char colour)
{
    for (int y = 0; y < VGA_HEIGHT; y++)
    {
        for (int x = 0; x < VGA_WIDTH; x++)
        {
            terminal_putchar(x, y, ' ', colour);
        }
    }

    terminal_row = 0;
    terminal_column = 0;
}
