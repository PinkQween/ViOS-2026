#ifndef VECTOR_H
#define VECTOR_H

/**
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file vector.h
 * @brief Dynamic array (vector) data structure and API.
 * @author Hanna Skairipa
 * @date 2026-05-10
 */

#include <stddef.h>
#include <stdint.h>

#include "status.h"

/**
 * @brief Vector reorder comparison callback.
 *
 * Return values:
 *  - > 0 : first element should come after second element
 *  - < 0 : first element should come before second element
 *  - = 0 : elements are equal
 */
typedef int (*VECTOR_REORDER_FUNCTION)(
    const void* first_element,
    const void* second_element
);

/**
 * @brief Vector configuration flags.
 */
enum
{
    VECTOR_NO_FLAGS = 0
};

/**
 * @brief Generic dynamically resizing vector.
 */
struct vector
{
    /**
     * @brief Pointer to vector memory buffer.
     */
    void* memory;

    /**
     * @brief Vector configuration flags.
     */
    int flags;

    /**
     * @brief Size of a single vector element.
     */
    size_t e_size;

    /**
     * @brief Total active elements stored in the vector.
     */
    size_t t_elems;

    /**
     * @brief Total allocated element capacity.
     */
    size_t tm_elems;

    /**
     * @brief Number of additional elements reserved during resize.
     */
    size_t t_reserved_elements;
};

/**
 * @brief Creates a new vector.
 *
 * @param element_size
 * Size of each vector element.
 *
 * @param total_reserved_elements_per_resize
 * Additional elements reserved whenever the vector grows.
 *
 * @param flags
 * Vector configuration flags.
 *
 * @return
 * Pointer to newly created vector on success, NULL on failure.
 */
struct vector* vector_new(
    size_t element_size,
    size_t total_reserved_elements_per_resize,
    int flags
);

/**
 * @brief Frees a vector and its backing memory.
 *
 * @param vec
 * Vector to free.
 */
void vector_free(struct vector* vec);

/**
 * @brief Resizes the vector if additional space is required.
 *
 * @param vec
 * Target vector.
 *
 * @param total_needed_elements
 * Number of additional elements required.
 *
 * @return
 * 0 on success, negative error code on failure.
 */
status_t vector_resize(
    struct vector* vec,
    size_t total_needed_elements
);

/**
 * @brief Pushes an element into the vector.
 *
 * The element memory is copied into the vector.
 *
 * @param vec
 * Target vector.
 *
 * @param elem
 * Pointer to element data.
 *
 * @return
 * Index of inserted element on success,
 * negative error code on failure.
 */
status_t vector_push(struct vector* vec, void* elem);

/**
 * @brief Removes the last element from the vector.
 *
 * @param vec
 * Target vector.
 *
 * @return
 * 0 on success, negative error code on failure.
 */
status_t vector_pop(struct vector* vec);

/**
 * @brief Retrieves the last element from the vector.
 *
 * @param vec
 * Target vector.
 *
 * @param data_out
 * Output buffer.
 *
 * @param size
 * Size of output buffer.
 *
 * @return
 * 0 on success, negative error code on failure.
 */
status_t vector_back(
    struct vector* vec,
    void* data_out,
    size_t size
);

/**
 * @brief Retrieves an element at the specified index.
 *
 * @param vec
 * Target vector.
 *
 * @param index
 * Element index.
 *
 * @param data_out
 * Output buffer.
 *
 * @param size
 * Size of output buffer.
 *
 * @return
 * 0 on success, negative error code on failure.
 */
status_t vector_at(
    struct vector* vec,
    size_t index,
    void* data_out,
    size_t size
);

/**
 * @brief Overwrites an element at the specified index.
 *
 * @param vec
 * Target vector.
 *
 * @param index
 * Index to overwrite.
 *
 * @param elem
 * Pointer to replacement element.
 *
 * @param elem_size
 * Size of replacement element.
 *
 * @return
 * 0 on success, negative error code on failure.
 */
status_t vector_overwrite(struct vector* vec,
                     int index,
                     void* elem,
                     size_t elem_size);

/**
 * @brief Removes the first matching element from the vector.
 *
 * Elements after the removed element are shifted left.
 *
 * @param vec
 * Target vector.
 *
 * @param mem_val
 * Pointer to element value to remove.
 *
 * @param size
 * Size of element.
 *
 * @return
 * 0 on success, negative error code on failure.
 */
status_t vector_delete(
    struct vector* vec,
    void* mem_val,
    size_t size
);

/**
 * @brief Reorders vector elements using a comparison callback.
 *
 * @param vec
 * Target vector.
 *
 * @param reorder_function
 * Comparison callback.
 */
void vector_reorder(
    struct vector* vec,
    VECTOR_REORDER_FUNCTION reorder_function
);

/**
 * @brief Returns the number of active elements in the vector.
 *
 * @param vec
 * Target vector.
 *
 * @return
 * Total number of elements.
 */
size_t vector_count(struct vector* vec);

#endif /* VECTOR_H */