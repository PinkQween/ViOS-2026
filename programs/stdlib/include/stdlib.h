#ifndef STDLIB_H
#define STDLIB_H

#include "stdint.h"

void* malloc(size_t size);
void free(void* ptr);

char* itoa(int i, char* str, int base);

#endif /* STDLIB_H */