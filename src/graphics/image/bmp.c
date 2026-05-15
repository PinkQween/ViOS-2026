#include "graphics/image/bmp.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "string/string.h"

struct image* bmp_image_load(void* memory, size_t size)
{
    struct image* img = NULL;
    struct bmp_header* header = NULL;
    struct bmp_image_header* bmp_image_header = NULL;

    if (size < sizeof(struct bmp_header))
        return NULL;

    header = (struct bmp_header*)memory;

    if (memcmp(header->type, BMP_SIGNATURE, sizeof(header->type)) != 0)
        return NULL;

    if (header->offbits >= size)
        return NULL;

    bmp_image_header = (struct bmp_image_header*)((uintptr_t)header+sizeof(struct bmp_header));

    img = kzalloc(sizeof(struct image));

    if (!img)
        return NULL;

    bool bottom_up = (bmp_image_header->height > 0);
    int32_t height = bottom_up ? bmp_image_header->height : -bmp_image_header->height;

    int32_t width = bmp_image_header->width;

    if (width <= 0 || height <= 0)
    {
        kfree(img);
        return NULL;
    }

    img->width = width;
    img->height = height;

    size_t pixel_data_size = (size_t)width * (size_t)height * sizeof(image_pixel_data);

    img->data = kzalloc(pixel_data_size);


    if (!img->data)
    {
        kfree(img);
        return NULL;
    }



    size_t raw_row_size = (size_t)width * bmp_image_header->bits_per_pixel / 8;
    size_t padded_row_size = (raw_row_size + 3) & ~3;

    if ((header->offbits + (padded_row_size * (size_t)height)) > size)
    {
        kfree(img->data);
        kfree(img);
        return NULL;
    }

    uint8_t* bmp_first_pixel_ptr = (uint8_t*)memory+header->offbits;

    for (size_t row = 0; row < height; row++)
    {
        uint8_t* row_ptr = bmp_first_pixel_ptr + (row * padded_row_size);
        int dest_row = bottom_up ? (height - row - 1) : row;
    
        for (size_t col = 0; col < width; col++)
        {
            uint8_t* bmp_pixel = row_ptr + (col * 3);
            uint8_t blue = bmp_pixel[0];
            uint8_t green = bmp_pixel[1];
            uint8_t red = bmp_pixel[2];    

            size_t pixel_index = (dest_row * (size_t)width) + (size_t)col;

            image_pixel_data* out_pixel = &((image_pixel_data*)img->data)[pixel_index];
        
            out_pixel->R = red;
            out_pixel->G = green;
            out_pixel->B = blue;
            out_pixel->A = 255;
        }
    }

    return img;
}

void bmp_image_free(struct image* img)
{
    kfree(img->data);
    kfree(img);
}
status_t bmp_image_format_register(struct image_format* format)
{
    (void)format;
    return STATUS_OK;
}

void bmp_image_format_unregister(struct image_format* format)
{
    (void)format;
}

struct image_format bmp_image_format = {
    .mime = "image/bmp",
    .image_load = bmp_image_load,
    .image_free = bmp_image_free,
    .on_register = bmp_image_format_register,
    .on_unregister = bmp_image_format_unregister,
};

struct image_format* graphics_image_format_bmp_setup()
{
    return &bmp_image_format;
}