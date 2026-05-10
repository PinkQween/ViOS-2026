#include "vector/vector.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"

#include "stddef.h"
#include "stdint.h"
#include "stdbool.h"

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

static long vector_offset(struct vector* vec, long index)
{
    return (long)(vec->e_size * index);
}

static status_t vector_valid_bounds(
    struct vector* vec,
    size_t index
)
{
    return (index < vec->t_elems)
        ? STATUS_OK
        : STATUS_ERR(EINVAL);
}

static void* vector_memory_at_index(
    struct vector* vec,
    size_t index
)
{
    long offset = vector_offset(vec, (long)index);

    return (void*)(
        (uintptr_t)vec->memory + (uintptr_t)offset
    );
}

struct vector* vector_new(
    size_t element_size,
    size_t total_reserved_elements_per_resize,
    int flags
)
{
    struct vector* vec =
        kpzalloc(sizeof(struct vector));

    if (!vec)
    {
        return NULL;
    }

    vec->e_size = element_size;
    vec->flags = flags;
    vec->t_elems = 0;
    vec->t_reserved_elements =
        total_reserved_elements_per_resize;
    vec->tm_elems = 0;
    vec->memory = NULL;

    return vec;
}

status_t vector_resize(
    struct vector* vec,
    size_t total_needed_elements
)
{
    if (!vec)
    {
        return STATUS_ERR(EINVAL);
    }

    size_t total_elements_required =
        vec->t_elems + total_needed_elements;

    if (vec->tm_elems >= total_elements_required)
    {
        return STATUS_OK;
    }

    size_t final_total_elements_required =
        total_elements_required +
        vec->t_reserved_elements;

    size_t total_bytes_required =
        final_total_elements_required *
        vec->e_size;

    void* new_memory =
        krealloc(vec->memory, total_bytes_required);

    if (!new_memory)
    {
        return STATUS_ERR(ENOMEM);
    }

    vec->memory = new_memory;
    vec->tm_elems = final_total_elements_required;

    return STATUS_OK;
}

status_t vector_push(
    struct vector* vec,
    void* elem
)
{
    if (!vec || !elem)
    {
        return STATUS_ERR(EINVAL);
    }

    status_t res = vector_resize(vec, 1);

    if (status_is_error(res))
    {
        return res;
    }

    size_t inserted_index = vec->t_elems;

    memcpy(
        vector_memory_at_index(vec, inserted_index),
        elem,
        vec->e_size
    );

    vec->t_elems++;

    return (status_t)inserted_index;
}

status_t vector_overwrite(
    struct vector* vec,
    int index,
    void* elem,
    size_t elem_size
)
{
    if (!vec || !elem)
    {
        return STATUS_ERR(EINVAL);
    }

    status_t res =
        vector_valid_bounds(vec, (size_t)index);

    if (status_is_error(res))
    {
        return res;
    }

    if (elem_size < vec->e_size)
    {
        return STATUS_ERR(EINVAL);
    }

    memcpy(
        vector_memory_at_index(vec, (size_t)index),
        elem,
        vec->e_size
    );

    return STATUS_OK;
}

status_t vector_pop(struct vector* vec)
{
    if (!vec || vec->t_elems == 0)
    {
        return STATUS_ERR(EIO);
    }

    vec->t_elems--;

    return STATUS_OK;
}

status_t vector_back(
    struct vector* vec,
    void* data_out,
    size_t size
)
{
    if (!vec || vec->t_elems == 0)
    {
        return STATUS_ERR(EIO);
    }

    return vector_at(
        vec,
        vec->t_elems - 1,
        data_out,
        size
    );
}

void vector_reorder(
    struct vector* vec,
    VECTOR_REORDER_FUNCTION reorder_function
)
{
    if (!vec || !reorder_function)
    {
        return;
    }

    size_t count = vector_count(vec);

    if (count < 2)
    {
        return;
    }

    uint8_t* elem1 = kzalloc(vec->e_size);
    uint8_t* elem2 = kzalloc(vec->e_size);

    if (!elem1 || !elem2)
    {
        if (elem1)
        {
            kfree(elem1);
        }

        if (elem2)
        {
            kfree(elem2);
        }

        return;
    }

    for (size_t i = 0; i < count - 1; i++)
    {
        for (size_t j = 0; j < count - i - 1; j++)
        {
            if (vector_at(
                    vec,
                    j,
                    elem1,
                    vec->e_size
                ) < 0)
            {
                goto cleanup;
            }

            if (vector_at(
                    vec,
                    j + 1,
                    elem2,
                    vec->e_size
                ) < 0)
            {
                goto cleanup;
            }

            if (reorder_function(elem1, elem2) > 0)
            {
                if (vector_overwrite(
                        vec,
                        (int)j,
                        elem2,
                        vec->e_size
                    ) < 0)
                {
                    goto cleanup;
                }

                if (vector_overwrite(
                        vec,
                        (int)(j + 1),
                        elem1,
                        vec->e_size
                    ) < 0)
                {
                    goto cleanup;
                }
            }
        }
    }

cleanup:
    kfree(elem1);
    kfree(elem2);
}

status_t vector_at(
    struct vector* vec,
    size_t index,
    void* data_out,
    size_t size
)
{
    if (!vec || !data_out)
    {
        return STATUS_ERR(EINVAL);
    }

    if (size < vec->e_size)
    {
        memset(data_out, 0, size);
        return STATUS_ERR(EINVAL);
    }

    status_t res =
        vector_valid_bounds(vec, index);

    if (status_is_error(res))
    {
        memset(data_out, 0, size);
        return res;
    }

    memcpy(
        data_out,
        vector_memory_at_index(vec, index),
        vec->e_size
    );

    return STATUS_OK;
}

status_t vector_delete(
    struct vector* vec,
    void* mem_val,
    size_t size
)
{
    status_t res = STATUS_OK;

    if (!vec || !mem_val)
    {
        return STATUS_ERR(EINVAL);
    }

    if (size != vec->e_size)
    {
        return STATUS_ERR(EINVAL);
    }

    size_t total_elements = vector_count(vec);

    long index_to_remove = -1;

    void* tmp_mem = kzalloc(size);

    if (!tmp_mem)
    {
        return STATUS_ERR(ENOMEM);
    }

    for (size_t i = 0; i < total_elements; i++)
    {
        res = vector_at(
            vec,
            i,
            tmp_mem,
            size
        );

        if (status_is_error(res))
        {
            goto out;
        }

        if (memcmp(tmp_mem, mem_val, size) == 0)
        {
            index_to_remove = (long)i;
            break;
        }
    }

    if (index_to_remove == -1)
    {
        res = STATUS_ERR(EIO);
        goto out;
    }

    if ((size_t)index_to_remove ==
        total_elements - 1)
    {
        res = vector_pop(vec);
        goto out;
    }

    uint8_t* dst =
        (uint8_t*)vec->memory +
        ((size_t)index_to_remove * vec->e_size);

    uint8_t* src =
        dst + vec->e_size;

    size_t elements_to_move =
        total_elements -
        ((size_t)index_to_remove + 1);

    memmove(
        dst,
        src,
        elements_to_move * vec->e_size
    );

    vec->t_elems--;

out:
    kfree(tmp_mem);
    return res;
}

size_t vector_count(struct vector* vec)
{
    if (!vec)
    {
        return 0;
    }

    return vec->t_elems;
}

void vector_free(struct vector* vec)
{
    if (!vec)
    {
        return;
    }

    if (vec->memory)
    {
        kfree(vec->memory);
    }

    kfree(vec);
}