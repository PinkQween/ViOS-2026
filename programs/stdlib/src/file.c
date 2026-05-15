#include "file.h"
#include "vios.h"

int fopen(const char* filename, const char* mode)
{
    return (int)vios_fopen(filename, mode);
}