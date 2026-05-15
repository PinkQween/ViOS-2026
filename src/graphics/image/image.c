#include "graphics/image/image.h"
#include "graphics/image/bmp.h"
#include "graphics/graphics.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "fs/file.h"
#include "vector/vector.h"
#include "string/string.h"

struct vector* image_formats;

void graphics_image_format_unload(struct image_format* format)
{
    if (format->on_unregister)
        format->on_unregister(format);
}

status_t graphics_image_format_register(struct image_format* format)
{
    struct image_format* exsisting_format = graphics_image_format_get(format->mime);
    status_t res = STATUS_OK;

    if (exsisting_format)
    {
        res = STATUS_ERR(EEXIST);
        goto out;
    }

    res = vector_push(image_formats, &format);

    if (status_is_error(res))
        goto out;

    if (format->on_register)
        res = format->on_register(format);

out:
    if (status_is_error(res))
    {
        if (format) 
        {
            graphics_image_format_unload(format);
        }
    }

    return res;
}

status_t graphics_image_formats_load()
{
    graphics_image_format_register(graphics_image_format_bmp_setup());
    return STATUS_OK;
}

void graphics_image_formats_unload()
{
    while (vector_count(image_formats) > 0)
    {
        struct image_format* format = NULL;

        vector_back(image_formats, &format, sizeof(struct image_format*));

        if (format)
            graphics_image_format_unload(format);
    
        vector_pop(image_formats);
    }
}

struct image_format* graphics_image_format_get(const char* mime)
{
    struct image_format* format = NULL;

    for (size_t i = 0; i < vector_count(image_formats); i++)
    {
        struct image_format* current_format = NULL;

        status_t res = vector_at(image_formats, i, &current_format, sizeof(struct image_format*));

        if (status_is_error(res))
            break;

        if (current_format && strncmp(current_format->mime, mime, sizeof(current_format->mime)) == 0)
        {
            format = current_format;
            break;
        }
    }

    return format;
}

struct image* graphics_image_load_from_memory(void* memory, size_t size)
{
    struct image* image_out = NULL;
    size_t total_formats = vector_count(image_formats);

    for (size_t i = 0; i < total_formats; i++)
    {
        struct image_format* format = NULL;

        status_t res = vector_at(image_formats, i, &format, sizeof(struct image_format*));

        if (status_is_error(res))
            break;

        image_out = format->image_load(memory, size);

        if (image_out) {
            image_out->format = format;
            break;
        }
    }

    return image_out;
}

image_pixel_data graphics_image_get_pixel(struct image* image, int x, int y)
{
    if (!image || !image->data || x < 0 || y < 0 || (uint32_t)x >= image->width || (uint32_t)y >= image->height)
    {
        image_pixel_data empty = {0};
        return empty;
    }

    return image->data[y * image->width + x];
}

void graphics_image_free(struct image* img)
{
    img->format->image_free(img);
}

struct image* graphics_image_load(const char* path)
{
    struct image* img = NULL;
    void* img_memory = NULL;

    status_t fd = fopen(path, "r");

    if (status_is_error(fd))
    {
        return NULL;
    }

    struct file_stat stat;

    status_t op_res = fstat(fd, &stat);

    if (status_is_error(op_res))
    {
        fclose(fd);
        return NULL;
    }

    img_memory = kzalloc(stat.size);

    if (!img_memory)
    {
        fclose(fd);
        return NULL;
    }

    op_res = fread(img_memory, stat.size, 1, fd);

    if (status_is_error(op_res))
    {
        kfree(img_memory);
        fclose(fd);
        return NULL;
    }

    img = graphics_image_load_from_memory(img_memory, stat.size);

    if (!img)
    {
        fclose(fd);
        kfree(img_memory);
        return NULL;
    }

    fclose(fd);
    kfree(img_memory);
    return img;
}

status_t graphics_image_formats_init()
{
    image_formats = vector_new(sizeof(struct image_format*), 10, 0);

    if (!image_formats)
    {
        return STATUS_ERR(ENOMEM);
    }
    
    status_t res = graphics_image_formats_load();

    if (status_is_error(res))
    {
        if (image_formats)
        {
            vector_free(image_formats);
        }
    }

    return res;
}