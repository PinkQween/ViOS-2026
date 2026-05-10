#ifndef PS2_KEYBOARD_H
#define PS2_KEYBOARD_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file ps2.h
 * @brief PS/2 keyboard driver interface.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#define PS2_PORT 0x64
#define PS2_COMMAND_ENABLE_FIRST_PORT 0xAE

#define PS2_KEYBOARD_KEY_RELEASED_BIT 0x80
#define ISR_KEYBOARD_INTERRUPT_VECTOR 0x21
#define KEYBOARD_INPUT_PORT 0x60
#define PS2_KEYBOARD_CAPSLOCK_SCANCODE 0x3A
#define PS2_KEYBOARD_LEFT_SHIFT_SCANCODE 0x2A
#define PS2_KEYBOARD_RIGHT_SHIFT_SCANCODE 0x36

struct interrupt_frame;

/**
 * Initializes the PS/2 keyboard, setting up any necessary data structures and configuring the keyboard controller for use. This function should be called during system initialization to ensure that the PS/2 keyboard is ready for use when the system starts running.
 * @return A pointer to the initialized PS/2 keyboard structure, or 0 on failure.
 */
struct keyboard* ps2_init();

/**
 * Handle a PS/2 keyboard interrupt.
 *
 * @param frame Saved interrupt frame.
 * @return None.
 */
void ps2_keyboard_handle_interrupt(struct interrupt_frame* frame);

#endif /* PS2_KEYBOARD_H */
