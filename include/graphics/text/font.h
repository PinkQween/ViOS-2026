#ifndef FONT_H
#define FONT_H

#include "stdint.h"
#include "stddef.h"
#include "stdbool.h"
#include "graphics/graphics.h"
#include "config.h"
#include "status.h"

#define FONT_IMAGE_DRAW_SUBTRACT_FROM_INDEX 32
#define FONT_IMAGE_CHAR_WIDTH_PIXEL_SIZE 9
#define FONT_IMAGE_CHAR_HEIGHT_PIXEL_SIZE 16
#define FONT_IMAGE_CHAR_Y_OFFSET 4

struct font
{
    size_t character_count;
    
    uint8_t* character_data;

    size_t bits_width_per_character;
    size_t bits_height_per_character;

    uint8_t subtract_from_ascii_char_index_for_drawing;

    char filename[MAX_PATH];
};

status_t font_system_init();
status_t font_draw(struct graphics_info* graphics_info, struct font* font, size_t screen_x, size_t screen_y, int character, struct framebuffer_pixel colour);
status_t font_draw_text(struct graphics_info* graphics_info, struct font* font, size_t screen_x, size_t screen_y, const char* text, struct framebuffer_pixel colour);
struct font* font_load(const char* filename);
struct font* font_get_loaded_font(const char* filename);
struct font* font_get_system_font();

#endif /* FONT_H */