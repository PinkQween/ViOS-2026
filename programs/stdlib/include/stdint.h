#ifndef STDINT_H
#define STDINT_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file stdint.h
 * @brief User-space fixed-width integer types.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

typedef signed char int8_t;
typedef unsigned char uint8_t;

typedef signed short int16_t;
typedef unsigned short uint16_t;

typedef signed int int32_t;
typedef unsigned int uint32_t;

typedef signed long long int64_t;
typedef unsigned long long uint64_t;

typedef unsigned int size_t;
typedef signed int intptr_t;
typedef unsigned int uintptr_t;

#endif /* STDINT_H */