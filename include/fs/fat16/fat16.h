#ifndef FAT16_H
#define FAT16_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file fat16.h
 * @brief FAT16 filesystem registration interface.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include "fs/file.h"

/**
 * Initialize the FAT16 filesystem descriptor.
 *
 * @return Pointer to the FAT16 filesystem instance used by the VFS layer.
 */
struct filesystem* fat16_init();

#endif /* FAT16_H */