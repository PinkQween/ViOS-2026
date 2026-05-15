#ifndef BMP_H
#define BMP_H


#include "status.h"
#include "graphics/image/image.h"
#include "stdint.h"
#include "stddef.h"
#include "status.h"

#define BIT_PER_PIXEL_MONOCHROME 1
#define BIT_PER_PIXEL_16_COLORS 4
#define BIT_PER_PIXEL_256_COLORS 8
#define BIT_PER_PIXEL_65536_COLORS 16
#define BIT_PER_PIXEL_16777216_COLORS 24

#define BMP_SIGNATURE "BM"

#define BMP_UNCOMPRESSION_UNCOMPRESSED 0
#define BMP_UNCOMPRESSION_RLE8 1
#define BMP_UNCOMPRESSION_RLE4 2
#define BMP_UNCOMPRESSION_BITFIELDS 3

struct bmp_header
{
    char type[2];
    uint32_t size;
    uint16_t reserved1;
    uint16_t reserved2;
    uint32_t offbits;
} __attribute__((packed));

struct bmp_image_header
{
    uint32_t size;
    uint32_t width;
    uint32_t height;
    uint16_t planes;
    uint16_t bits_per_pixel;
    uint32_t compression;
    uint32_t size_image;
    uint32_t x_pixels_per_m;
    uint32_t y_pixels_per_m;
    uint32_t colors_used_count;
    uint32_t important_colors_count;
} __attribute__((packed));


struct color_table
{
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t reserved;
};

struct image_format* graphics_image_format_bmp_setup();

#endif /* BMP_H */