#ifndef CONSOLE_H
#define CONSOLE_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file console.h
 * @brief Text console and VGA output interface.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include "status.h"

/** Width of text-mode VGA framebuffer in characters. */
#define VGA_WIDTH 80

/** Height of text-mode VGA framebuffer in characters. */
#define VGA_HEIGHT 25

/** VGA text-mode colour values. */
#define BLACK 0x00
#define BLUE 0x01
#define GREEN 0x02
#define CYAN 0x03
#define RED 0x04
#define MAGENTA 0x05
#define BROWN 0x06
#define LIGHT_GREY 0x07
#define DARK_GREY 0x08
#define LIGHT_BLUE 0x09
#define LIGHT_GREEN 0x0A
#define LIGHT_CYAN 0x0B
#define LIGHT_RED 0x0C
#define LIGHT_MAGENTA 0x0D
#define YELLOW 0x0E
#define WHITE 0x0F

/**
 * Initialize kernel text output over serial and VGA.
 *
 * @return None.
 */
void kernel_console_init();

/**
 * Print a null-terminated string to the kernel output.
 *
 * @param str Text to print.
 * @return None.
 */
void print(const char *str);

/**
 * Print one character to the kernel output.
 *
 * @param c Character to print.
 * @return None.
 */
void print_char(char c);

/**
 * Print a null-terminated string using a VGA text-mode colour byte.
 *
 * @param str Text to print.
 * @param colour Combined foreground/background colour byte.
 * @return None.
 */
void print_w_color(const char *str, char colour);

/**
 * Halt normal flow and report a fatal kernel error.
 *
 * @param message Panic message to display.
 * @return None.
 */
void panic(const char *message);

/**
 * Print a failed status with a short context message and halt.
 *
 * @param message Context for the failed operation.
 * @param status Negative status_t error to report.
 * @return None.
 */
void panic_status(const char *message, status_t status);

/**
 * Combine a VGA foreground and background colour.
 *
 * @param fg Foreground colour (0-15).
 * @param bg Background colour (0-15).
 * @return Combined colour byte for VGA text mode.
 */
char choose_colour(char fg, char bg);

/**
 * Clear the VGA terminal and reset the cursor to the top-left cell.
 *
 * @param colour Combined foreground/background colour byte to apply.
 * @return None.
 */
void terminal_clear_color_and_reset_cursor(char colour);

#endif /* CONSOLE_H */
