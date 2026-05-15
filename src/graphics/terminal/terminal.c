#include "graphics/terminal/terminal.h"
#include "graphics/text/font.h"
#include "graphics/graphics.h"
#include "graphics/image/image.h"
#include "memory/heap/kheap.h"
#include "vector/vector.h"
#include "memory/memory.h"
#include "status.h"
#include "stdbool.h"
#include "stddef.h"
#include "stdint.h"

struct vector* terminal_vector = NULL;

static inline size_t terminal_abs_x_for_next_character(struct terminal* terminal)
{
    return terminal->bounds.abs_x + (terminal->text.col * terminal->font->bits_width_per_character);
}

static inline size_t terminal_abs_y_for_next_character(struct terminal* terminal)
{
    return terminal->bounds.abs_y + (terminal->text.row * terminal->font->bits_height_per_character);
}

void terminal_system_setup()
{
    terminal_vector = vector_new(sizeof(struct terminal*), 4, 0);
}

struct terminal* terminal_create(struct graphics_info* graphics_info, int starting_x, int starting_y, size_t width, size_t height, struct font* font, struct framebuffer_pixel font_color, int flags)
{
    if (font == NULL || graphics_info == NULL)
    {
        return NULL;
    }

    if (starting_x < 0 || starting_y < 0 || 
        starting_x >= (int)graphics_info->horizontal_resolution ||
        starting_y >= (int)graphics_info->vertical_resolution)
    {
        return NULL;
    }

    struct terminal* terminal = kzalloc(sizeof(struct terminal));
    if (!terminal)
    {
        return NULL;
    }

    terminal->graphics_info = graphics_info;
    terminal->terminal_background = NULL;
    terminal->text.row = 0;
    terminal->text.col = 0;
    terminal->bounds.abs_x = starting_x;
    terminal->bounds.abs_y = starting_y;
    terminal->bounds.width = width;
    terminal->bounds.height = height;
    terminal->font = font;
    terminal->font_color = font_color;
    terminal->back_color = (struct framebuffer_pixel){0, 0, 0, 0}; 
    terminal->flags = flags;

    terminal_background_save(terminal);

    if (terminal_vector)
    {
        vector_push(terminal_vector, &terminal);
    }

    return terminal;
}

void terminal_free(struct terminal* terminal)
{
    if (!terminal)
        return;

    if (terminal->terminal_background)
    {
        kfree(terminal->terminal_background);
        terminal->terminal_background = NULL;
    }

    if (terminal_vector)
    {
        vector_delete(terminal_vector, &terminal, sizeof(struct terminal*));
    }

    kfree(terminal);
}

struct terminal* terminal_get_at_screen_position(size_t x, size_t y, struct terminal* ignore_terminal)
{
    if (!terminal_vector)
        return NULL;

    struct terminal* found_terminal = NULL;
    size_t total_children = vector_count(terminal_vector);

    for (size_t i = 0; i < total_children; i++)
    {
        struct terminal* terminal = NULL;
        vector_at(terminal_vector, i, &terminal, sizeof(terminal));

        if (terminal == ignore_terminal)
            continue;

        if (x >= terminal->bounds.abs_x &&
            x < terminal->bounds.abs_x + terminal->bounds.width &&
            y >= terminal->bounds.abs_y &&
            y < terminal->bounds.abs_y + terminal->bounds.height)
        {
            found_terminal = terminal;
            break;
        }
    }

    return found_terminal;
}

void terminal_background_save(struct terminal* terminal)
{
    if (!terminal || !terminal->graphics_info || !terminal->graphics_info->pixels)
        return;

    size_t width = terminal->bounds.width;
    size_t height = terminal->bounds.height;

    size_t total_pixels = width * height;
    size_t buffer_size = total_pixels * sizeof(struct framebuffer_pixel);

    if (!terminal->terminal_background)
    {
        terminal->terminal_background = kzalloc(buffer_size);
        if (!terminal->terminal_background)
            return;
    }

    struct framebuffer_pixel* pixel_buffer = terminal->graphics_info->pixels;
    size_t graphics_width = terminal->graphics_info->width;

    for (size_t y = 0; y < height; y++)
    {
        for (size_t x = 0; x < width; x++)
        {
            size_t src_x = terminal->bounds.abs_x + x;
            size_t src_y = terminal->bounds.abs_y + y;
            terminal->terminal_background[y * width + x] = pixel_buffer[src_y * graphics_width + src_x];
        }
    }
}

static void terminal_handle_newline(struct terminal* terminal)
{
    terminal->text.row++;
    size_t total_rows_per_term = terminal_total_rows(terminal);
    if (terminal->text.row >= total_rows_per_term)
    {
        terminal->text.row = 0;
    }
    terminal->text.col = 0;
}

static void terminal_update_position_after_draw(struct terminal* terminal)
{
    terminal->text.col++;
    size_t total_cols_per_row = terminal_total_cols(terminal);
    size_t total_rows_per_term = terminal_total_rows(terminal);
    if (terminal->text.col >= total_cols_per_row)
    {
        terminal->text.col = 0;
        terminal->text.row++;
    }
    if (terminal->text.row >= total_rows_per_term)
    {
        terminal->text.col = 0;
        terminal->text.row = 0;
    }
}

int terminal_cursor_set(struct terminal* terminal, int row, int col)
{
    if (!terminal) return -1;
    size_t total_cols_per_row = terminal_total_cols(terminal);
    size_t total_rows_per_term = terminal_total_rows(terminal);
    if (col < 0 || (size_t)col >= total_cols_per_row) return -1;
    if (row < 0 || (size_t)row >= total_rows_per_term) return -1;
    terminal->text.row = row;
    terminal->text.col = col;
    return 0;
}

int terminal_cursor_row(struct terminal* terminal) { return terminal ? (int)terminal->text.row : -1; }
int terminal_cursor_col(struct terminal* terminal) { return terminal ? (int)terminal->text.col : -1; }
int terminal_total_cols(struct terminal* terminal) { return (!terminal || !terminal->font) ? 0 : (int)(terminal->bounds.width / terminal->font->bits_width_per_character); }
int terminal_total_rows(struct terminal* terminal) { return (!terminal || !terminal->font) ? 0 : (int)(terminal->bounds.height / terminal->font->bits_height_per_character); }

static bool terminal_bounds_check(struct terminal* terminal, size_t abs_x, size_t abs_y)
{
    return (abs_x >= terminal->bounds.abs_x && abs_x < terminal->bounds.abs_x + terminal->bounds.width &&
            abs_y >= terminal->bounds.abs_y && abs_y < terminal->bounds.abs_y + terminal->bounds.height);
}

void terminal_restore_background(struct terminal* terminal, int sx, int sy, int width, int height)
{
    if (!terminal || !terminal->terminal_background) return;
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            size_t rel_x = sx + x;
            size_t rel_y = sy + y;
            if (rel_x >= terminal->bounds.width || rel_y >= terminal->bounds.height) continue;
            size_t abs_x = terminal->bounds.abs_x + rel_x;
            size_t abs_y = terminal->bounds.abs_y + rel_y;
            graphics_draw_pixel(terminal->graphics_info, abs_x, abs_y, terminal->terminal_background[rel_y * terminal->bounds.width + rel_x]);
        }
    }
    graphics_redraw_graphics_to_screen(terminal->graphics_info, terminal->bounds.abs_x + sx, terminal->bounds.abs_y + sy, width, height);
}

int terminal_backspace(struct terminal* terminal)
{
    if (!terminal || !(terminal->flags & TERMINAL_FLAG_BACKSPACE_ALLOWED)) return 0;
    int current_col = terminal_cursor_col(terminal) - 1;
    int current_row = terminal_cursor_row(terminal);
    if (current_col < 0) {
        current_col = terminal_total_cols(terminal) - 1;
        current_row--;
    }
    if (current_row < 0) { current_row = 0; current_col = 0; }
    terminal_cursor_set(terminal, current_row, current_col);
    size_t rel_x = terminal->text.col * terminal->font->bits_width_per_character;
    size_t rel_y = terminal->text.row * terminal->font->bits_height_per_character;
    terminal_restore_background(terminal, rel_x, rel_y, terminal->font->bits_width_per_character, terminal->font->bits_height_per_character);
    return 0;
}

int terminal_write(struct terminal* terminal, int c)
{
    if (!terminal) return -1;
    if (c == '\n') { terminal_handle_newline(terminal); return 0; }
    if (c == 0x08 && (terminal->flags & TERMINAL_FLAG_BACKSPACE_ALLOWED)) { terminal_backspace(terminal); return 0; }
    size_t abs_x = terminal_abs_x_for_next_character(terminal);
    size_t abs_y = terminal_abs_y_for_next_character(terminal);
    graphics_draw_rect(terminal->graphics_info, abs_x, abs_y, terminal->font->bits_width_per_character, terminal->font->bits_height_per_character, terminal->back_color);
    font_draw(terminal->graphics_info, terminal->font, abs_x, abs_y, c, terminal->font_color);
    terminal_update_position_after_draw(terminal);
    return 0;
}

int terminal_pixel_set(struct terminal* terminal, size_t x, size_t y, struct framebuffer_pixel pixel_color)
{
    if (!terminal) return -1;
    size_t abs_x = terminal->bounds.abs_x + x;
    size_t abs_y = terminal->bounds.abs_y + y;
    if (!terminal_bounds_check(terminal, abs_x, abs_y)) return -1;
    graphics_draw_pixel(terminal->graphics_info, abs_x, abs_y, pixel_color);
    return 0;
}

int terminal_draw_image(struct terminal* terminal, uint32_t x, uint32_t y, struct image* img)
{
    if (!terminal || !img) return -1;
    size_t abs_x = terminal->bounds.abs_x + x;
    size_t abs_y = terminal->bounds.abs_y + y;
    if (!terminal_bounds_check(terminal, abs_x, abs_y)) return -1;
    graphics_draw_image(terminal->graphics_info, img, abs_x, abs_y);
    return 0;
}

int terminal_draw_rect(struct terminal* terminal, uint32_t x, uint32_t y, size_t width, size_t height, struct framebuffer_pixel pixel_color)
{
    if (!terminal) return -1;
    size_t abs_x = terminal->bounds.abs_x + x;
    size_t abs_y = terminal->bounds.abs_y + y;
    if (!terminal_bounds_check(terminal, abs_x, abs_y)) return -1;
    graphics_draw_rect(terminal->graphics_info, abs_x, abs_y, width, height, pixel_color);
    graphics_redraw_graphics_to_screen(terminal->graphics_info, abs_x, abs_y, width, height);
    return 0;
}

void terminal_clear(struct terminal* terminal, struct framebuffer_pixel bg_color)
{
    if (!terminal) return;

    terminal->back_color = bg_color;
    terminal_cursor_set(terminal, 0, 0);

    graphics_draw_rect(
        terminal->graphics_info,
        terminal->bounds.abs_x,
        terminal->bounds.abs_y,
        terminal->bounds.width,
        terminal->bounds.height,
        bg_color
    );

    if (terminal->terminal_background)
    {
        size_t total_pixels = terminal->bounds.width * terminal->bounds.height;
        for (size_t i = 0; i < total_pixels; i++)
        {
            terminal->terminal_background[i] = bg_color;
        }
    }
    else
    {
        terminal_background_save(terminal);
    }

    if (terminal->bounds.abs_x == 0 &&
        terminal->bounds.abs_y == 0 &&
        terminal->bounds.width == terminal->graphics_info->width &&
        terminal->bounds.height == terminal->graphics_info->height)
    {
        graphics_clear_hardware(bg_color);
    }

    graphics_redraw_graphics_to_screen(
        terminal->graphics_info,
        terminal->bounds.abs_x,
        terminal->bounds.abs_y,
        terminal->bounds.width,
        terminal->bounds.height
    );
}

int terminal_print(struct terminal* terminal, const char* message)
{
    if (!terminal || !message) return -1;
    while (*message != 0) {
        int res = terminal_write(terminal, *message);
        if (res < 0) return res;
        message++;
    }
    return 0;
}
