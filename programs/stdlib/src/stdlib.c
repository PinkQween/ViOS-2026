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

char* itoa(int i, char* str, int base) {
    if (base < 2 || base > 16) return 0;

    char* ptr = str;
    char* ptr1 = str;
    unsigned int u;

    if (i < 0 && base == 10) {
        *ptr++ = '-';
        u = (unsigned int)(-(i + 1)) + 1;
    } else {
        u = (unsigned int)i;
    }

    do {
        unsigned int rem = u % base;
        *ptr++ = "0123456789abcdef"[rem];
        u /= base;
    } while (u);

    *ptr = '\0';
    
    if (str[0] == '-') ptr1++;
    char* end = ptr - 1;
    while (ptr1 < end) {
        char tmp = *ptr1;
        *ptr1++ = *end;
        *end-- = tmp;
    }

    return str;
}
