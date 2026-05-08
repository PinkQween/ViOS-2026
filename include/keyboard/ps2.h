#ifndef PS2_KEYBOARD_H
#define PS2_KEYBOARD_H

#define PS2_PORT 0x64
#define PS2_COMMAND_ENABLE_FIRST_PORT 0xAE

#define PS2_KEYBOARD_KEY_RELEASED_BIT 0x80
#define ISR_KEYBOARD_INTERRUPT_VECTOR 0x21
#define KEYBOARD_INPUT_PORT 0x60

struct interrupt_frame;

/**
 * Initializes the PS/2 keyboard, setting up any necessary data structures and configuring the keyboard controller for use. This function should be called during system initialization to ensure that the PS/2 keyboard is ready for use when the system starts running.
 * @return A pointer to the initialized PS/2 keyboard structure, or 0 on failure.
 */
struct keyboard* ps2_init();

void ps2_keyboard_handle_interrupt(struct interrupt_frame* frame);

#endif /* PS2_KEYBOARD_H */
