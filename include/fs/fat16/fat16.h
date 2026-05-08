#ifndef FAT16_H
#define FAT16_H

#include "fs/file.h"

/**
 * Initialize the FAT16 filesystem descriptor.
 *
 * @return Pointer to the FAT16 filesystem instance used by the VFS layer.
 */
struct filesystem* fat16_init();

#endif /* FAT16_H */