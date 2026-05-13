#ifndef STDDEF_H
#define STDDEF_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file stddef.h
 * @brief Freestanding standard definition types.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#define NULL ((void*)0)

typedef unsigned long long size_t;
typedef signed long long ptrdiff_t;

#define offsetof(type, member) __builtin_offsetof(type, member)

#endif /* STDDEF_H */
