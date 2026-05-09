#include "keyboard/ps2.h"
#include "keyboard/keyboard.h"
#include "idt/idt.h"
#include "io/io.h"
#include "kernel.h"
#include "task/task.h"

#include "stddef.h"
#include "stdint.h"

static uint8_t keyboard_scan_set_one[] = {
    0x00, 0x1B, '1', '2', '3', '4', '5',
    '6', '7', '8', '9', '0', '-', '=',
    0x08, '\t', 'q', 'w', 'e', 'r', 't',
    'y', 'u', 'i', 'o', 'p', '[', ']',
    0x0d, 0x00, 'a', 's', 'd', 'f', 'g',
    'h', 'j', 'k', 'l', ';', '\'', '`',
    0x00, '\\', 'z', 'x', 'c', 'v', 'b',
    'n', 'm', ',', '.', '/', 0x00, '*',
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, '7', '8', '9', '-', '4', '5',
    '6', '+', '1', '2', '3', '0', '.'
};

static bool left_shift_pressed = false;
static bool right_shift_pressed = false;

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

    keyboard_set_caps_lock(&ps2_keyboard, false);

    outb(PS2_PORT, PS2_COMMAND_ENABLE_FIRST_PORT); // Enable PS/2 keyboard
    return 0;
}

static bool ps2_keyboard_is_shift_scancode(uint8_t scancode)
{
    return scancode == PS2_KEYBOARD_LEFT_SHIFT_SCANCODE ||
           scancode == PS2_KEYBOARD_RIGHT_SHIFT_SCANCODE;
}

static bool ps2_keyboard_shift_pressed()
{
    return left_shift_pressed || right_shift_pressed;
}

static char ps2_keyboard_apply_shift(char c)
{
    if (c >= 'a' && c <= 'z') {
        return c - ('a' - 'A');
    }

    switch (c) {
        case '1': return '!';
        case '2': return '@';
        case '3': return '#';
        case '4': return '$';
        case '5': return '%';
        case '6': return '^';
        case '7': return '&';
        case '8': return '*';
        case '9': return '(';
        case '0': return ')';
        case '-': return '_';
        case '=': return '+';
        case '[': return '{';
        case ']': return '}';
        case '\\': return '|';
        case ';': return ':';
        case '\'': return '"';
        case '`': return '~';
        case ',': return '<';
        case '.': return '>';
        case '/': return '?';
        default: return c;
    }
}

uint8_t ps2_keyboard_scancode_to_char(uint8_t scancode)
{
    size_t size_of_keyboard_scan_set_one = sizeof(keyboard_scan_set_one) / sizeof(uint8_t);
    
    if (scancode >= size_of_keyboard_scan_set_one) {
        return 0;
    }

    if (scancode == PS2_KEYBOARD_CAPSLOCK_SCANCODE) {
        keyboard_set_caps_lock(&ps2_keyboard, !keyboard_get_caps_lock(&ps2_keyboard));
        return 0;
    }
    
    char c = keyboard_scan_set_one[scancode];

    if (c >= 'a' && c <= 'z') {
        if (keyboard_get_caps_lock(&ps2_keyboard) != ps2_keyboard_shift_pressed()) {
            c = c - ('a' - 'A');
        }
    } else if (ps2_keyboard_shift_pressed()) {
        c = ps2_keyboard_apply_shift(c);
    }

    return c;
}

void ps2_keyboard_handle_interrupt(struct interrupt_frame* frame)
{
    uint8_t scancode = inb(KEYBOARD_INPUT_PORT);

    uint8_t pressed_scancode = scancode & ~PS2_KEYBOARD_KEY_RELEASED_BIT;
    if (ps2_keyboard_is_shift_scancode(pressed_scancode)) {
        bool pressed = !(scancode & PS2_KEYBOARD_KEY_RELEASED_BIT);

        if (pressed_scancode == PS2_KEYBOARD_LEFT_SHIFT_SCANCODE) {
            left_shift_pressed = pressed;
        } else {
            right_shift_pressed = pressed;
        }

        return;
    }
    
    if (scancode & PS2_KEYBOARD_KEY_RELEASED_BIT) {
        return;
    }

    uint8_t c = ps2_keyboard_scancode_to_char(scancode);

    if (c) {
        keyboard_push(c);
    }
}

struct keyboard* ps2_init()
{
    return &ps2_keyboard;
}
