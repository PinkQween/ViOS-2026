#include "graphics/graphics.h"
#include "kernel.h"
#include "console/console.h"
#include "memory/paging/paging.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "vector/vector.h"
#include "status.h"

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
    if (!src_info)
    {
        return;
    }

    uint32_t src_x_end = src_x + width;
    uint32_t src_y_end = src_y + height;

    if (src_x_end > src_info->width)
        src_x_end = src_info->width;

    if (src_y_end > src_info->height)
        src_y_end = src_info->height;

    uint32_t final_w = src_x_end - src_x;
    uint32_t final_h = src_y_end - src_y;

    if (final_w == 0 || final_h == 0)
    {
        return;
    }

    struct graphics_info* screen = graphics_screen_info();

    uint32_t screen_w = screen->horizontal_resolution;
    uint32_t screen_h = screen->vertical_resolution;

    if (dest_x >= screen_w || dest_y >= screen_h)
    {
        return;
    }

    uint32_t dest_x_end = dest_x + final_w;
    uint32_t dest_y_end = dest_y + final_h;

    if (dest_x_end > screen_w)
        dest_x_end = screen_w;

    if (dest_y_end > screen_h)
        dest_y_end = screen_h;

    uint32_t clipped_w = dest_x_end - dest_x;
    uint32_t clipped_h = dest_y_end - dest_y;

    if (clipped_w == 0 || clipped_h == 0)
    {
        return;
    }

    for (uint32_t y = 0; y < clipped_h; y++)
    {
        for (uint32_t x = 0; x < clipped_w; x++)
        {
            struct framebuffer_pixel pixel = src_info->pixels[(src_y + y) * src_info->width + (src_x + x)];
            struct framebuffer_pixel no_transparency_color = {0};

            // do we have transparency key set, and does this pixel match the transparency key
            if (memcmp(&src_info->transparency_key, &no_transparency_color, sizeof(src_info->transparency_key)) != 0)
            {
                if (memcmp(&pixel, &src_info->transparency_key, sizeof(src_info->transparency_key)) == 0)
                {
                    continue;
                }
            }

            screen->framebuffer[(dest_y + y) * screen->pixels_per_scanline + (dest_x + x)] = pixel;
        }
    }
}

void graphics_draw_pixel(struct graphics_info* graphics_info, uint32_t x, uint32_t y, struct framebuffer_pixel pixel)
{
    struct framebuffer_pixel black_pixel = {0};

    if (memcmp(&graphics_info->ignore_color, &black_pixel, sizeof(graphics_info->ignore_color)) != 0)
    {
        if (memcmp(&graphics_info->ignore_color, &pixel, sizeof(graphics_info->ignore_color)) == 0)
        {
            return;
        }
    }
    
    // transparency only calulated duruing redraw, so we can ignore it here

    if (x < graphics_info->width && y < graphics_info->height)
    {
        graphics_info->pixels[y * graphics_info->width + x] = pixel;
    }
}

void graphics_redraw_only(struct graphics_info* graphics_info)
{
    if (!graphics_info)
    {
        return;
    }
    
    graphics_paste_pixels_to_framebuffer(
        graphics_info,
        0,
        0,
        graphics_info->width,
        graphics_info->height,
        graphics_info->starting_x,
        graphics_info->starting_y
    );
}

void graphics_redraw(struct graphics_info* graphics_info)
{
    if (!graphics_info)
    {
        return;
    }

    graphics_redraw_only(graphics_info);
}

void graphics_redraw_all()
{
    graphics_redraw(graphics_screen_info());
}

void graphics_setup(struct graphics_info* main_graphics_info)
{
    if (loaded_graphics_info)
    {
        panic("The graphics system has already been initialized");
    }

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
    main_graphics_info->relative_x = 0;
    main_graphics_info->relative_y = 0;
    main_graphics_info->starting_x = 0;
    main_graphics_info->starting_y = 0;

    void* virt_map_start = paging_align_to_lower_page(new_framebuffer_memory);
    void* phys_map_start = paging_align_to_lower_page(real_framebuffer);
    void* phys_map_end = paging_align_address((void*)((uintptr_t)real_framebuffer + framebuffer_size));

    status_t map_res = paging_map_to(
        kernel_desc(),
        virt_map_start,
        phys_map_start,
        phys_map_end,
        PAGING_IS_PRESENT | PAGING_IS_WRITEABLE
    );

    if (status_is_error(map_res))
    {
        panic_status("Failed to map framebuffer", map_res);
    }

    loaded_graphics_info = main_graphics_info;

    for (size_t y = 0; y < main_graphics_info->vertical_resolution; y++)
    {
        for (size_t x = 0; x < main_graphics_info->horizontal_resolution; x++)
        {
            struct framebuffer_pixel pixel = {0, 0, 0, 0};
            
            graphics_draw_pixel(main_graphics_info, x, y, pixel);
        }
    }

    graphics_info_vector = vector_new(sizeof(struct graphics_info*), 4, 0);

    vector_push(graphics_info_vector, &main_graphics_info);

    // TODO: Load text formats
}