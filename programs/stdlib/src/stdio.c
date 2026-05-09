
#include "stdio.h"
#include "stdlib.h"
#include "vios.h"
#include "stdarg.h"

int getchar()
{
    return vios_getkey();
}

int putchar(int c)
{
    vios_putchar((char)c);
    return c;
}

int printf(const char* format, ...)
{
    va_list args;
    const char* p;
    char* sval;
    int ival;

    va_start(args, format);

    for (p = (const char*)format; *p; p++) {
        if (*p != '%') {
            putchar(*p);
            continue;
        }

        switch (*++p) {
            case 'd':
                ival = va_arg(args, int);
                char buf[12];
                itoa(ival, buf, 10);
                vios_print(buf);
                break;
            case 's':
                sval = va_arg(args, char*);
                vios_print(sval);
                break;
            case 'c':
                ival = va_arg(args, int);
                putchar(ival);
                break;
            case '%':
                putchar('%');
                break;
            case 'i':
                ival = va_arg(args, int);
                char ibuf[12];
                itoa(ival, ibuf, 10);
                vios_print(ibuf);
                break;
            case 'x':
                ival = va_arg(args, int);
                char xbuf[12];
                itoa(ival, xbuf, 16);
                vios_print(xbuf);
                break;
            case 'b':
                ival = va_arg(args, int);
                char bbuf[33];
                itoa(ival, bbuf, 2);
                vios_print(bbuf);
                break;
            case 'u':
                ival = va_arg(args, int);
                char ubuf[12];
                itoa(ival, ubuf, 10);
                vios_print(ubuf);
                break;
            default:
                putchar('%');
                putchar(*p);
        }
    }

    va_end(args);
    return 0;
}