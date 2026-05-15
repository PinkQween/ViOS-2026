#include "graphics/graphics.h"
#include "graphics/image/image.h"
#include "kernel.h"
#include "console/console.h"
#include "memory/paging/paging.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "vector/vector.h"
#include "string/string.h"
#include "status.h"
#include "math.h"

struct graphics_info* loaded_graphics_info = NULL;
struct vector* graphics_info_vector = NULL;

void* real_framebuffer = NULL;
void* real_framebuffer_end = NULL;
size_t real_framebuffer_width = 0;
size_t real_framebuffer_height = 0;
size_t real_framebuffer_pixels_per_scanline = 0;

struct graphics_info* graphics_screen_info()
{
    return loaded_graphics_info;
}

void graphics_paste_pixels_to_framebuffer(
    struct graphics_info* src_info,
    uint32_t src_x,
    uint32_t src_y,
    uint32_t width,
    uint32_t height,
    uint32_t dest_x,
    uint32_t dest_y
)
{
    if (!src_info) return;

    uint32_t src_x_end = src_x + width;
    uint32_t src_y_end = src_y + height;
    if (src_x_end > src_info->width) src_x_end = src_info->width;
    if (src_y_end > src_info->height) src_y_end = src_info->height;

    uint32_t final_w = src_x_end - src_x;
    uint32_t final_h = src_y_end - src_y;
    if (final_w == 0 || final_h == 0) return;

    struct graphics_info* screen = graphics_screen_info();
    uint32_t screen_w = screen->horizontal_resolution;
    uint32_t screen_h = screen->vertical_resolution;

    if (dest_x >= screen_w || dest_y >= screen_h) return;

    uint32_t dest_x_end = dest_x + final_w;
    uint32_t dest_y_end = dest_y + final_h;
    if (dest_x_end > screen_w) dest_x_end = screen_w;
    if (dest_y_end > screen_h) dest_y_end = screen_h;

    uint32_t clipped_w = dest_x_end - dest_x;
    uint32_t clipped_h = dest_y_end - dest_y;
    if (clipped_w == 0 || clipped_h == 0) return;

    for (uint32_t y = 0; y < clipped_h; y++) {
        for (uint32_t x = 0; x < clipped_w; x++) {
            struct framebuffer_pixel pixel = src_info->pixels[(src_y + y) * src_info->width + (src_x + x)];
            screen->framebuffer[(dest_y + y) * screen->pixels_per_scanline + (dest_x + x)] = pixel;
        }
    }
}

void graphics_draw_pixel(struct graphics_info* graphics_info, uint32_t x, uint32_t y, struct framebuffer_pixel pixel)
{
    if (x < graphics_info->width && y < graphics_info->height) {
        graphics_info->pixels[y * graphics_info->width + x] = pixel;
    }
}

void graphics_draw_image(struct graphics_info* graphics_info, struct image* image, int x, int y)
{
    if (!image) return;
    if (!graphics_info) graphics_info = loaded_graphics_info;
    for (size_t ly = 0; ly < (size_t)image->height; ly++) {
        for (size_t lx = 0; lx < (size_t)image->width; lx++) {
            image_pixel_data* pixel_data = &((image_pixel_data*)image->data)[ly * image->width + lx];
            struct framebuffer_pixel pixel = { .blue = pixel_data->B, .green = pixel_data->G, .red = pixel_data->R, .reserved = 0 };
            graphics_draw_pixel(graphics_info, x + lx, y + ly, pixel);
        }
    }
}

void graphics_redraw_only(struct graphics_info* graphics_info)
{
    if (!graphics_info) return;
    graphics_paste_pixels_to_framebuffer(graphics_info, 0, 0, graphics_info->width, graphics_info->height, graphics_info->starting_x, graphics_info->starting_y);
}

void graphics_redraw_children(struct graphics_info* g)
{
    size_t total_children = vector_count(g->children);
    for (size_t i = 0; i < total_children; i++) {
        struct graphics_info* child = NULL;
        vector_at(g->children, i, &child, sizeof(child));
        if (child) graphics_redraw(child);
    }
}

void graphics_redraw_region(struct graphics_info* g, size_t local_x, size_t local_y, size_t width, size_t height)
{
    if (!g) return;
    if (local_x >= g->width || local_y >= g->height) return;
    if (local_x + width > g->width) width = g->width - local_x;
    if (local_y + height > g->height) height = g->height - local_y;

    graphics_paste_pixels_to_framebuffer(g, local_x, local_y, width, height, g->starting_x + local_x, g->starting_y + local_y);

    size_t region_abs_left = g->starting_x + local_x;
    size_t region_abs_top = g->starting_y + local_y;
    size_t region_abs_right = region_abs_left + width;
    size_t region_abs_bottom = region_abs_top + height;

    size_t child_count = vector_count(g->children);
    for (size_t i = 0; i < child_count; i++) {
        struct graphics_info* child = NULL;
        vector_at(g->children, i, &child, sizeof(child));
        if (!child) continue;

        size_t child_left = (size_t)child->starting_x;
        size_t child_top = (size_t)child->starting_y;
        size_t child_right = child_left + child->width;
        size_t child_bottom = child_top + child->height;

        size_t intersect_left = MAX(region_abs_left, child_left);
        size_t intersect_top = MAX(region_abs_top, child_top);
        size_t intersect_right = MIN(region_abs_right, child_right);
        size_t intersect_bottom = MIN(region_abs_bottom, child_bottom);

        if (intersect_right > intersect_left && intersect_bottom > intersect_top) {
            graphics_redraw_region(child, intersect_left - child_left, intersect_top - child_top, intersect_right - intersect_left, intersect_bottom - intersect_top);
        }
    }
}

void graphics_redraw_graphics_to_screen(struct graphics_info* relative_graphics, size_t x, size_t y, size_t width, size_t height)
{
    graphics_redraw_region(graphics_screen_info(), relative_graphics->starting_x + x, relative_graphics->starting_y + y, width, height);
}

void graphics_redraw(struct graphics_info* graphics_info)
{
    if (!graphics_info) return;
    graphics_redraw_only(graphics_info);
    graphics_redraw_children(graphics_info);
}

void graphics_redraw_all()
{
    graphics_redraw(graphics_screen_info());
}

void graphics_setup(struct graphics_info* main_graphics_info)
{
    if (loaded_graphics_info) panic("The graphics system has already been initialized");

    real_framebuffer = main_graphics_info->framebuffer;
    real_framebuffer_width = main_graphics_info->horizontal_resolution;
    real_framebuffer_height = main_graphics_info->vertical_resolution;
    real_framebuffer_pixels_per_scanline = main_graphics_info->pixels_per_scanline;

    size_t framebuffer_size = real_framebuffer_height * real_framebuffer_pixels_per_scanline * sizeof(struct framebuffer_pixel);
    real_framebuffer_end = (void*)((uintptr_t)real_framebuffer+framebuffer_size);

    void* new_framebuffer_memory = kzalloc(framebuffer_size);
    main_graphics_info->framebuffer = new_framebuffer_memory;
    main_graphics_info->children = vector_new(sizeof(struct graphics_info*), 4, 0);
    size_t pixel_buffer_size = (size_t)main_graphics_info->horizontal_resolution * (size_t)main_graphics_info->vertical_resolution * sizeof(struct framebuffer_pixel);
    main_graphics_info->pixels = kzalloc(pixel_buffer_size);
    main_graphics_info->width = main_graphics_info->horizontal_resolution;
    main_graphics_info->height = main_graphics_info->vertical_resolution;
    main_graphics_info->starting_x = 0;
    main_graphics_info->starting_y = 0;

    paging_map_to(kernel_desc(), (void*)((uintptr_t)new_framebuffer_memory & ~0xFFF), (void*)((uintptr_t)real_framebuffer & ~0xFFF), (void*)(((uintptr_t)real_framebuffer + framebuffer_size + 0xFFF) & ~0xFFF), PAGING_IS_PRESENT | PAGING_IS_WRITEABLE);

    loaded_graphics_info = main_graphics_info;
    graphics_info_vector = vector_new(sizeof(struct graphics_info*), 4, 0);
    vector_push(graphics_info_vector, &main_graphics_info);
    graphics_image_formats_init();

    struct graphics_info* g = graphics_screen_info();
    if (g && g->pixels) {
        memset(g->pixels, 0, (size_t)g->width * g->height * sizeof(struct framebuffer_pixel));
        graphics_redraw_all();
    }
}

void graphics_draw_rect(struct graphics_info* graphics_info, uint32_t x, uint32_t y, size_t width, size_t height, struct framebuffer_pixel pixel_color)
{
    if (!graphics_info) return;
    for (uint32_t ly = 0; ly < height; ly++) {
        for (uint32_t lx = 0; lx < width; lx++) {
            graphics_draw_pixel(graphics_info, x + lx, y + ly, pixel_color);
        }
    }
}

void graphics_ignore_color(struct graphics_info* graphics_info, struct framebuffer_pixel pixel_color) { (void)graphics_info; (void)pixel_color; }
void graphics_ignore_color_finish(struct graphics_info* graphics_info) { (void)graphics_info; }
void graphics_transparency_key_set(struct graphics_info* graphics_info, struct framebuffer_pixel pixel_color) { (void)graphics_info; (void)pixel_color; }
void graphics_transparency_key_remove(struct graphics_info* graphics_info) { (void)graphics_info; }

void graphics_clear_hardware(struct framebuffer_pixel color)
{
    struct graphics_info* screen = graphics_screen_info();
    if (!screen || !screen->framebuffer) return;
    uint32_t val = 0;
    memcpy(&val, &color, sizeof(struct framebuffer_pixel));
    size_t total_pixels = (size_t)screen->pixels_per_scanline * screen->vertical_resolution;
    uint32_t* fb = (uint32_t*)screen->framebuffer;
    for (size_t i = 0; i < total_pixels; i++) fb[i] = val;
}
