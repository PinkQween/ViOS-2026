#include "stdlib.h"
#include "vios.h"

void* malloc(size_t size)
{
    return vios_malloc(size);
}

void free(void* ptr)
{
    vios_free(ptr);
}