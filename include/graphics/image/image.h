#ifndef IMAGE_H
#define IMAGE_H

#include "status.h"
#include "stdint.h"
#include "stddef.h"

struct image_format;

typedef union image_pixel_data {
    uint32_t data;
    struct {
        uint8_t R;
        uint8_t G;
        uint8_t B;
        uint8_t A;
    };
} image_pixel_data;

struct image
{
    uint32_t width;
    uint32_t height;

    image_pixel_data* data;

    void* internal;

    struct image_format* format;
};

typedef struct image*(*IMAGE_LOAD_FUNCTION)(void* memory, size_t size);
typedef void(*IMAGE_FREE_FUNCTION)(struct image* img);
typedef status_t(*IMAGE_FORMAT_REGISTER_FUNCTION)(struct image_format* format);
typedef void(*IMAGE_FORMAT_UNREGISTER_FUNCTION)(struct image_format* format);

#define IMAGE_FORMAT_MAX_MIME_TYPE 64

struct image_format
{
    char mime[IMAGE_FORMAT_MAX_MIME_TYPE];
    
    IMAGE_LOAD_FUNCTION image_load;
    IMAGE_FREE_FUNCTION image_free;

    IMAGE_FORMAT_REGISTER_FUNCTION on_register;
    IMAGE_FORMAT_UNREGISTER_FUNCTION on_unregister;

    void* internal;
};

status_t graphics_image_formats_init();
struct image* graphics_image_load(const char* path);
image_pixel_data graphics_image_get_pixel(struct image* image, int x, int y);
struct image* graphics_image_load_from_memory(void* memory, size_t size);
void graphics_image_free(struct image* img);
struct image_format* graphics_image_format_get(const char* mime);

#endif /* IMAGE_H */