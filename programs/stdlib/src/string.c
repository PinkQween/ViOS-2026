#include "string.h"

char* sp = 0;

int strcmp(const char* s1, const char* s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

int strlen(const char* str)
{
    int len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

void* memcpy(void* dest, const void* src, size_t n)
{
    char* d = (char*)dest;
    const char* s = (const char*)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
    return dest;
}

void* memset(void* dest, int val, size_t n)
{
    char* d = (char*)dest;
    for (size_t i = 0; i < n; i++) {
        d[i] = (char)val;
    }
    return dest;
}

char* strcpy(char* dest, const char* src)
{
    char* d = dest;
    while (*src) {
        *d++ = *src++;
    }
    *d = '\0';
    return dest;
}

char* strtok(char* str, const char* delim)
{
    if (str) {
        sp = str;
    } else if (!sp) {
        return NULL;
    }

    char* token_start = sp;

    while (*sp) {
        const char* d = delim;
        while (*d) {
            if (*sp == *d) {
                *sp++ = '\0';
                return token_start;
            }
            d++;
        }
        sp++;
    }

    sp = NULL;
    return token_start;
}

char* strncpy(char* dest, const char* src, size_t n)
{
    char* d = dest;
    size_t i;
    for (i = 0; i < n && src[i]; i++) {
        d[i] = src[i];
    }
    if (i < n) {
        d[i] = '\0';
    }
    return dest;
}