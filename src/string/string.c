#include "string/string.h"
#include "console/console.h"

#include "stdarg.h"
#include "stdbool.h"

char tolower(char c)
{
    if (c >= 'A' && c <= 'Z')
    {
        return c + ('a' - 'A');
    }
 
    return c;
}

static void append_char(char *str, size_t size, size_t *written, char c)
{
    if (size > 0 && *written + 1 < size)
    {
        str[*written] = c;
    }

    (*written)++;
}

static void append_string(char *str, size_t size, size_t *written, const char *value)
{
    if (value == NULL)
    {
        value = "(null)";
    }

    for (size_t i = 0; value[i] != '\0'; i++)
    {
        append_char(str, size, written, value[i]);
    }
}

static void append_unsigned(char *str, size_t size, size_t *written, unsigned int value, unsigned int base, bool uppercase)
{
    char buffer[33];
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    size_t index = 0;

    if (value == 0)
    {
        append_char(str, size, written, '0');
        return;
    }

    while (value > 0 && index < sizeof(buffer))
    {
        buffer[index++] = digits[value % base];
        value /= base;
    }

    while (index > 0)
    {
        append_char(str, size, written, buffer[--index]);
    }
}

static void append_signed(char *str, size_t size, size_t *written, int value)
{
    unsigned int magnitude;

    if (value < 0)
    {
        append_char(str, size, written, '-');
        magnitude = (unsigned int)(-(value + 1)) + 1;
    }
    else
    {
        magnitude = (unsigned int)value;
    }

    append_unsigned(str, size, written, magnitude, 10, false);
}

static void append_padding(char *str, size_t size, size_t *written, size_t count, char pad)
{
    for (size_t i = 0; i < count; i++)
    {
        append_char(str, size, written, pad);
    }
}

size_t strlen(const char *str)
{
    size_t len = 0;
    while (str[len] != '\0')
    {
        len++;
    }

    return len;
}

size_t strnlen(const char *str, size_t maxlen)
{
    size_t len = 0;
    while (len < maxlen && str[len] != '\0')
    {
        len++;
    }

    return len;
}

bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}

char *itoa(int value, char *str, int base)
{
    char *ptr = str, *ptr1 = str, tmp_char;
    int tmp_value = 0;

    if (base < 2 || base > 36)
    {
        *str = '\0';
        return str;
    }

    if (value == 0)
    {
        str[0] = '0';
        str[1] = '\0';
        return str;
    }

    while (value) {
        tmp_value = value;
        value /= base;
        *ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
    };

    if (tmp_value < 0)
    {
        *ptr++ = '-';
    }

    *ptr-- = '\0';

    while (ptr1 < ptr)
    {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }

    return str;
}

int atoi(const char *str)
{
    int res = 0;
    int sign = 1;
    int i = 0;

    if (str[0] == '-')
    {
        sign = -1;
        i++;
    }

    for (; str[i] != '\0'; i++)
    {
        if (!is_digit(str[i]))
        {
            break;
        }
        res = res * 10 + str[i] - '0';
    }

    return sign * res;
}

static int vsnprintf_impl(char *str, size_t size, const char *format, va_list args)
{
    size_t written = 0;

    for (const char *spec = format; *spec != '\0'; spec++)
    {
        if (*spec != '%')
        {
            append_char(str, size, &written, *spec);
            continue;
        }

        spec++;
        if (*spec == '\0')
        {
            break;
        }

        if (*spec == '%')
        {
            append_char(str, size, &written, '%');
        }
        else
        {
            bool zero_pad = false;
            size_t width = 0;

            if (*spec == '0')
            {
                zero_pad = true;
                spec++;
            }

            while (*spec >= '0' && *spec <= '9')
            {
                width = (width * 10) + (size_t)(*spec - '0');
                spec++;
            }

            if (*spec == 'd' || *spec == 'i')
            {
                char buffer[32];
                int value = va_arg(args, int);
                bool negative = value < 0;
                size_t digits = 0;
                unsigned int magnitude = negative
                    ? (unsigned int)(-(value + 1)) + 1
                    : (unsigned int)value;

                do {
                    buffer[digits++] = (char)('0' + (magnitude % 10));
                    magnitude /= 10;
                } while (magnitude > 0 && digits < sizeof(buffer));

                size_t total = digits + (negative ? 1 : 0);
                if (width > total)
                {
                    append_padding(str, size, &written, width - total, zero_pad ? '0' : ' ');
                }

                if (negative)
                {
                    append_char(str, size, &written, '-');
                }

                while (digits > 0)
                {
                    append_char(str, size, &written, buffer[--digits]);
                }
            }
            else if (*spec == 'u')
            {
                char buffer[32];
                unsigned int value = va_arg(args, unsigned int);
                size_t digits = 0;

                do {
                    buffer[digits++] = (char)('0' + (value % 10));
                    value /= 10;
                } while (value > 0 && digits < sizeof(buffer));

                if (width > digits)
                {
                    append_padding(str, size, &written, width - digits, zero_pad ? '0' : ' ');
                }

                while (digits > 0)
                {
                    append_char(str, size, &written, buffer[--digits]);
                }
            }
            else if (*spec == 'x' || *spec == 'X')
            {
                char buffer[32];
                const char *digits_map = (*spec == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
                unsigned int value = va_arg(args, unsigned int);
                size_t digits = 0;

                do {
                    buffer[digits++] = digits_map[value % 16];
                    value /= 16;
                } while (value > 0 && digits < sizeof(buffer));

                if (width > digits)
                {
                    append_padding(str, size, &written, width - digits, zero_pad ? '0' : ' ');
                }

                while (digits > 0)
                {
                    append_char(str, size, &written, buffer[--digits]);
                }
            }
            else if (*spec == 'c')
            {
                append_char(str, size, &written, (char)va_arg(args, int));
            }
            else if (*spec == 's')
            {
                append_string(str, size, &written, va_arg(args, const char *));
            }
            else
            {
                append_char(str, size, &written, '%');
                if (zero_pad)
                {
                    append_char(str, size, &written, '0');
                }

                if (width > 0)
                {
                    char width_buffer[16];
                    size_t width_digits = 0;
                    size_t temp = width;

                    do {
                        width_buffer[width_digits++] = (char)('0' + (temp % 10));
                        temp /= 10;
                    } while (temp > 0 && width_digits < sizeof(width_buffer));

                    while (width_digits > 0)
                    {
                        append_char(str, size, &written, width_buffer[--width_digits]);
                    }
                }

                append_char(str, size, &written, *spec);
            }
        }
    }

    if (size > 0)
    {
        size_t terminator_index = written < size ? written : size - 1;
        str[terminator_index] = '\0';
    }

    return (int)written;
}

int printf(const char *format, ...)
{
    char buffer[1024];

    va_list args;
    va_start(args, format);
    int written = vsnprintf_impl(buffer, sizeof(buffer), format, args);
    va_end(args);

    print(buffer);
    return written;
}

int snprintf(char *str, size_t size, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    int written = vsnprintf_impl(str, size, format, args);
    va_end(args);

    return written;
}

int sscanf(const char *str, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    int assigned = 0;
    const char *input = str;
    const char *spec = format;

    while (*spec != '\0')
    {
        if (*spec != '%')
        {
            if (*input != *spec)
            {
                break;
            }

            input++;
            spec++;
            continue;
        }

        spec++;
        if (*spec == '\0')
        {
            break;
        }

        if (*spec != 'c')
        {
            while (*input == ' ' || *input == '\t' || *input == '\n' || *input == '\r' || *input == '\v' || *input == '\f')
            {
                input++;
            }
        }

        if (*spec == 'd' || *spec == 'i')
        {
            int sign = 1;
            int value = 0;

            if (*input == '+' || *input == '-')
            {
                if (*input == '-')
                {
                    sign = -1;
                }
                input++;
            }

            if (!is_digit(*input))
            {
                break;
            }

            while (is_digit(*input))
            {
                value = value * 10 + (*input - '0');
                input++;
            }

            int *out = va_arg(args, int *);
            *out = sign * value;
            assigned++;
        }
        else if (*spec == 'x' || *spec == 'X')
        {
            int value = 0;
            int digits = 0;

            if (input[0] == '0' && (input[1] == 'x' || input[1] == 'X'))
            {
                input += 2;
            }

            while ((*input >= '0' && *input <= '9') || (*input >= 'a' && *input <= 'f') || (*input >= 'A' && *input <= 'F'))
            {
                char c = *input;
                int digit = 0;

                if (c >= '0' && c <= '9')
                {
                    digit = c - '0';
                }
                else if (c >= 'a' && c <= 'f')
                {
                    digit = 10 + (c - 'a');
                }
                else
                {
                    digit = 10 + (c - 'A');
                }

                value = (value * 16) + digit;
                input++;
                digits++;
            }

            if (digits == 0)
            {
                break;
            }

            int *out = va_arg(args, int *);
            *out = value;
            assigned++;
        }
        else if (*spec == 'u')
        {
            unsigned int value = 0;
            int digits = 0;

            if (!is_digit(*input))
            {
                break;
            }

            while (is_digit(*input))
            {
                value = (value * 10) + (unsigned int)(*input - '0');
                input++;
                digits++;
            }

            if (digits == 0)
            {
                break;
            }

            unsigned int *out = va_arg(args, unsigned int *);
            *out = value;
            assigned++;
        }
        else if (*spec == 's')
        {
            char *out = va_arg(args, char *);
            int copied = 0;

            if (*input == '\0')
            {
                break;
            }

            while (*input != '\0' && *input != ' ' && *input != '\t' && *input != '\n' && *input != '\r' && *input != '\v' && *input != '\f')
            {
                out[copied++] = *input++;
            }

            out[copied] = '\0';
            if (copied == 0)
            {
                break;
            }

            assigned++;
        }
        else if (*spec == 'c')
        {
            char *out = va_arg(args, char *);
            if (*input == '\0')
            {
                break;
            }

            *out = *input++;
            assigned++;
        }
        else if (*spec == '%')
        {
            if (*input != '%')
            {
                break;
            }

            input++;
        }
        else
        {
            break;
        }

        spec++;
    }

    va_end(args);
    return assigned;
}

char* strcpy(char *dest, const char *src)
{
    char *ptr = dest;

    while (*src != '\0')
    {
        *ptr++ = *src++;
    }

    *ptr = '\0';
    return dest;
}

char *strncpy(char *dest, const char *src, size_t n)
{
    char *ptr = dest;
    size_t i = 0;

    while (i < n && src[i] != '\0')
    {
        *ptr++ = src[i++];
    }

    if (i < n)
    {
        *ptr = '\0';
    }

    return dest;
}

int strncmp(const char *s1, const char *s2, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        if (s1[i] != s2[i])
        {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }

        if (s1[i] == '\0')
        {
            return 0;
        }
    }

    return 0;
}

int strcmp(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }

    return (unsigned char)*s1 - (unsigned char)*s2;
}

int strlen_terminator(const char *str, int max, char terminator)
{
    int i = 0;
    for (i = 0; i < max && str[i] != '\0' && str[i] != terminator; i++);
    return i;
}

int istrncmp(const char *s1, const char *s2, size_t n)
{
    for (size_t i = 0; i < n; i++)
    {
        char c1 = s1[i];
        char c2 = s2[i];

        if (c1 >= 'A' && c1 <= 'Z')
        {
            c1 += 'a' - 'A';
        }

        if (c2 >= 'A' && c2 <= 'Z')
        {
            c2 += 'a' - 'A';
        }

        if (c1 != c2)
        {
            return (unsigned char)c1 - (unsigned char)c2;
        }

        if (c1 == '\0')
        {
            return 0;
        }
    }

    return 0;
}
