#include "keyboard/ps2.h"
#include "keyboard/keyboard.h"
#include "idt/idt.h"
#include "io/io.h"
#include "kernel.h"
#include "task/task.h"

#include <stddef.h>
#include <stdint.h>

static uint8_t keyboard_scan_set_one[] = {
    0x00, 0x1B, '1', '2', '3', '4', '5',
    '6', '7', '8', '9', '0', '-', '=',
    0x08, '\t', 'Q', 'W', 'E', 'R', 'T',
    'Y', 'U', 'I', 'O', 'P', '[', ']',
    0x0d, 0x00, 'A', 'S', 'D', 'F', 'G',
    'H', 'J', 'K', 'L', ';', '\'', '`', 
    0x00, '\\', 'Z', 'X', 'C', 'V', 'B',
    'N', 'M', ',', '.', '/', 0x00, '*',
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, '7', '8', '9', '-', '4', '5',
    '6', '+', '1', '2', '3', '0', '.'
};

int ps2_keyboard_init();

struct keyboard ps2_keyboard = {
    .init = ps2_keyboard_init,
    .name = "PS/2 Keyboard"
};

int ps2_keyboard_init()
{
    status_t res = idt_register_interrupt_callback(0x21, ps2_keyboard_handle_interrupt);
    
    if (status_is_error(res)) {
        return res;
    }

    outb(PS2_PORT, PS2_COMMAND_ENABLE_FIRST_PORT); // Enable PS/2 keyboard
    return 0;
}

uint8_t ps2_keyboard_scancode_to_char(uint8_t scancode)
{
    size_t size_of_keyboard_scan_set_one = sizeof(keyboard_scan_set_one) / sizeof(uint8_t);
    
    if (scancode >= size_of_keyboard_scan_set_one) {
        return 0;
    }
    
    char c = keyboard_scan_set_one[scancode];
    
    return c;
}

void ps2_keyboard_handle_interrupt(struct interrupt_frame* frame)
{
    kernel_page();
    uint8_t scancode = inb(KEYBOARD_INPUT_PORT);
    inb(KEYBOARD_INPUT_PORT); // Acknowledge the interrupt
    
    if (scancode & PS2_KEYBOARD_KEY_RELEASED_BIT) {
        return;
    }

    uint8_t c = ps2_keyboard_scancode_to_char(scancode);

    if (c) {
        keyboard_push(c);
    }

    task_page();
}

struct keyboard* ps2_init()
{
    return &ps2_keyboard;
}
