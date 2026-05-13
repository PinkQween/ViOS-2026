#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "stdint.h"
#include "stdbool.h"
#include "vector/vector.h"

enum
{
    GRAPHICS_FLAG_ALLOW_OUT_OF_BOUNDS                   = 0b00000001,
    GRAPHICS_FLAG_CLONED_FRAMEBUFFER                    = 0b00000100,
    GRAPHICS_FLAG_CLONED_CHILDREN                       = 0b00001000,
    GRAPHICS_FLAG_DO_NOT_COPY_PIXELS                    = 0b00010000,
    GRAPHICS_FLAG_DO_NOT_OVERWRITE_TRANSPARENT_PIXELS   = 0b00100000,
};

struct graphics_info;

typedef void(*GRAPHICS_MOUSE_CLICK_FUNCTION)(struct graphics_info* graphics, size_t x, size_t y, int click_type);
typedef void(*GRAPHICS_MOUSE_MOVE_FUNCTION)(struct graphics_info* graphics, size_t absolute_x, size_t absolute_y, size_t relative_x, size_t relative_y);

struct framebuffer_pixel
{
    uint8_t blue;
    uint8_t green;
    uint8_t red;
    uint8_t reserved;
};

struct graphics_info
{
    struct framebuffer_pixel* framebuffer;
    uint32_t horizontal_resolution;
    uint32_t vertical_resolution;
    uint32_t pixels_per_scanline;

    struct framebuffer_pixel* pixels;

    uint32_t width;
    uint32_t height;

    uint32_t starting_x;
    uint32_t starting_y;

    uint32_t relative_x;
    uint32_t relative_y;

    struct graphics_info* parent;
    struct vector* children;

    uint32_t flags;
    uint32_t z_index;

    struct framebuffer_pixel ignore_color;

    struct framebuffer_pixel transparency_key;

    struct
    {
        GRAPHICS_MOUSE_CLICK_FUNCTION mouse_click;
        GRAPHICS_MOUSE_MOVE_FUNCTION mouse_move;
    } event_hanlders;
};

void graphics_draw_pixel(struct graphics_info* graphics_info, uint32_t x, uint32_t y, struct framebuffer_pixel pixel);
void graphics_redraw(struct graphics_info* graphics_info);
struct graphics_info* graphics_screen_info();
void graphics_setup(struct graphics_info* main_graphics_info);
void graphics_redraw_all();

#endif /* GRAPHICS_H */