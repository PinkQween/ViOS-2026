#include "graphics/text/font.h"
#include "graphics/graphics.h"
#include "graphics/image/image.h"
#include "memory/heap/kheap.h"
#include "vector/vector.h"
#include "string/string.h"
#include "memory/memory.h"
#include "console/console.h"

struct vector* loaded_fonts = NULL;
struct font* system_font = NULL;

struct font* font_get_system_font()
{
    return system_font;
}

struct font* font_create(uint8_t* character_data, size_t character_count, size_t bits_width_per_character, size_t bits_height_per_character, uint8_t subtract_from_ascii_char_index_for_drawing)
{
    struct font* font = kzalloc(sizeof(struct font));

    if (!font)
    {
        return NULL;
    }

    font->character_count = character_count;
    font->character_data = character_data;
    font->bits_width_per_character = bits_width_per_character;
    font->bits_height_per_character = bits_height_per_character;
    font->subtract_from_ascii_char_index_for_drawing = subtract_from_ascii_char_index_for_drawing;

    return font;
}

struct font* font_load_from_image(const char* filename, size_t pixel_width, size_t pixel_height, size_t y_offset_per_character)
{
    struct image* img_font = graphics_image_load(filename);

    if (!img_font) {
        return NULL;
    }

    size_t characters_per_row = img_font->width / pixel_width;
    size_t total_rows = img_font->height / pixel_height;
    size_t total_characters = characters_per_row * total_rows;

    size_t total_required_bits_for_character_set = total_characters * pixel_width * pixel_height;
    size_t total_required_bytes_for_character_set = total_required_bits_for_character_set / 8;

    if (total_required_bits_for_character_set % 8)
    {
        total_required_bytes_for_character_set++;
    }

    size_t total_required_bits_per_character = pixel_width * pixel_height;
    size_t total_required_bytes_per_character = total_required_bits_per_character / 8;

    if (total_required_bits_per_character % 8)
    {
        total_required_bytes_per_character++;
    }

    uint8_t* character_data = kzalloc(total_required_bytes_for_character_set);

    if (!character_data)
    {
        graphics_image_free(img_font);
        return NULL;
    }

    for (size_t row = 0; row < total_rows; row++)
    {
        for (size_t col = 0; col < characters_per_row; col++)
        {
            size_t character_index = row * characters_per_row + col;
            size_t starting_x = col * pixel_width;
            size_t starting_y = row * pixel_height;

            for (size_t x = 0; x < pixel_width; x++)
            {
                for (size_t y = y_offset_per_character; y < pixel_height; y++)
                {
                    size_t abs_x = starting_x + x;
                    size_t abs_y = starting_y + y;

                    image_pixel_data pixel = graphics_image_get_pixel(img_font, abs_x, abs_y);

                    if (pixel.R != 0 || pixel.B != 0 || pixel.G != 0)
                    {
                        size_t char_offset = character_index * total_required_bytes_per_character;
                        size_t bit_index = y * pixel_width + x;
                        size_t byte_index = char_offset + bit_index / 8;
                        uint8_t bit_mask = 1 << (bit_index % 8);

                        character_data[byte_index] |= bit_mask;
                    }
                }
            }
        }
    }

    return font_create(character_data, total_characters, pixel_width, pixel_height, FONT_IMAGE_DRAW_SUBTRACT_FROM_INDEX);
}

struct font* font_get_loaded_font(const char* filename)
{
    struct font* font = NULL;

    size_t total_fonts = vector_count(loaded_fonts);

    for (size_t i = 0; i < total_fonts; i++)
    {
        vector_at(loaded_fonts, i, &font, sizeof(struct font*));

        if (font && strncmp(font->filename, filename, sizeof(font->filename)) == 0)
        {
            return font;
        }
    }

    return NULL;
}

struct font* font_load(const char* filename)
{
    struct font* font = NULL;
    
    if (font)
    {
        return font;
    }

    size_t pixel_width = FONT_IMAGE_CHAR_WIDTH_PIXEL_SIZE;
    size_t pixel_height = FONT_IMAGE_CHAR_HEIGHT_PIXEL_SIZE;
    size_t y_offset = FONT_IMAGE_CHAR_Y_OFFSET;

    font = font_load_from_image(filename, pixel_width, pixel_height, y_offset);

    if (font)
    {
        strncpy(font->filename, filename, sizeof(font->filename));
        vector_push(loaded_fonts, &font);
    }

    return font;
}

status_t font_draw_from_index(struct graphics_info* graphics_info, struct font* font, size_t screen_x, size_t screen_y,  int index_character, struct framebuffer_pixel colour)
{
    if (index_character < 0 || (size_t)index_character >= font->character_count)
    {
        return STATUS_ERR(EINVAL);
    }

    size_t bits_per_character = font->bits_width_per_character * font->bits_height_per_character;
    size_t bytes_per_character = bits_per_character / 8;
    if (bits_per_character % 8)
        bytes_per_character++;

    size_t char_offset = index_character * bytes_per_character;

    for (size_t y = 0; y < font->bits_height_per_character; y++)
    {
        for (size_t x = 0; x < font->bits_width_per_character; x++)
        {
            size_t bit_index = y * font->bits_width_per_character + x;
            size_t byte_index = char_offset + (bit_index / 8);
            uint8_t bit_mask = 1 << (bit_index % 8);

            if (font->character_data[byte_index] & bit_mask)
            {
                graphics_draw_pixel(graphics_info, screen_x + x, screen_y + y, colour);
            }
        }
    }


    graphics_redraw_graphics_to_screen(graphics_info, screen_x, screen_y, font->bits_width_per_character, font->bits_height_per_character);

    return STATUS_OK;
}

status_t font_draw(struct graphics_info* graphics_info, struct font* font, size_t screen_x, size_t screen_y, int character, struct framebuffer_pixel colour)
{
    int index_character = (int)((unsigned char)character) - (int)font->subtract_from_ascii_char_index_for_drawing;

    return font_draw_from_index(graphics_info, font, screen_x, screen_y, index_character, colour);
}

status_t font_draw_text(struct graphics_info* graphics_info, struct font* font, size_t screen_x, size_t screen_y, const char* text, struct framebuffer_pixel colour)
{
    size_t len = strnlen(text, 1024);

    for (size_t i = 0; i < len; i++)
    {
        char c = text[i];

        if (c == '\n')
        {
            screen_y += font->bits_height_per_character;
            screen_x = 0;
            continue;
        }

        status_t res = font_draw(graphics_info, font, screen_x, screen_y, c, colour);

        if (status_is_error(res))
        {
            return res;
        }

        screen_x += font->bits_width_per_character;
    }

    return STATUS_OK;
}

status_t font_system_init()
{
    loaded_fonts = vector_new(sizeof(struct font*), 4, 0);

    if (!loaded_fonts)
    {
        return STATUS_ERR(ENOMEM);
    }

    system_font = font_load("/fonts/sysfont.bmp");

    if (!system_font)
    {
        panic("Failed to load system font");
    }

    return STATUS_OK;
}