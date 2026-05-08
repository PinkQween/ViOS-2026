#ifndef VIOS_H
#define VIOS_H

#include <stddef.h>
#include <stdint.h>

void vios_print(const char* str);
int vios_getkey();
void vios_putchar(char c);
void* vios_malloc(size_t size);
void vios_free(void* ptr);

#endif
