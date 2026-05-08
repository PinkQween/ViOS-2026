#ifndef DISK_H
#define DISK_H

#include "status.h"
#include "fs/file.h"

/** Backend type for a disk device. */
typedef unsigned int DISK_TYPE;

/** Physical/real disk backend. */
#define DISK_TYPE_REAL 0

/** Global disk descriptor used by low-level IO and VFS. */
struct disk {
    /** Disk backend type. */
    DISK_TYPE type;
    /** Device sector size in bytes. */
    int sector_size;
    
    /** Disk id (used in paths and streamer creation). */
    int id;

    /** Filesystem resolved for this disk, if any. */
    struct filesystem* fs;

    /** Filesystem-private runtime context pointer. */
    void* fs_internal;
};

/**
 * Probe available disks and initialize the global disk table.
 *
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t disk_search_and_init();

/**
 * Retrieve a disk descriptor by index/id.
 *
 * @param index Disk id/index.
 * @return Disk descriptor, or NULL if not found.
 */
struct disk* disk_get(int index);

/**
 * Read one or more sectors from disk into buffer.
 *
 * @param idisk Disk descriptor.
 * @param lba Starting logical block address.
 * @param total Number of sectors to read.
 * @param buffer Output buffer.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t disk_read_block(struct disk* idisk, int lba, int total, void* buffer);

#endif /* DISK_H */
