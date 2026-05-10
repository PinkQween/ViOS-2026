#ifndef STDARG_H
#define STDARG_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file stdarg.h
 * @brief Freestanding variable argument support.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

typedef __builtin_va_list va_list;

#define va_start(v, l) __builtin_va_start(v, l)
#define va_end(v)       __builtin_va_end(v)
#define va_arg(v, l)    __builtin_va_arg(v, l)
#define va_copy(d, s)    __builtin_va_copy(d, s)

#endif