#ifndef DISK_H
#define DISK_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file disk.h
 * @brief Kernel disk device abstraction.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include "status.h"
#include "fs/file.h"
#include "stdint.h"

/** Backend type for a disk device. */
typedef unsigned int DISK_TYPE;

/** Physical/real disk backend. */
#define DISK_TYPE_REAL 0
/** Virtual/partition disk backend. */
#define DISK_TYPE_PARTITION 1

/** Global disk descriptor used by low-level IO and VFS. */
struct disk {
    /** Disk backend type. */
    DISK_TYPE type;
    /** Device sector size in bytes. */
    int sector_size;

    /** First accessible LBA for this disk mapping. */
    uint64_t starting_lba;
    /** Last accessible LBA for this disk mapping (inclusive). */
    uint64_t ending_lba;
    
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
 * Create and register a disk descriptor.
 *
 * @param type Disk backend type.
 * @param starting_lba First accessible LBA.
 * @param ending_lba Last accessible LBA (inclusive), or 0 for unbounded.
 * @param sector_size Sector size for this disk mapping.
 * @param disk_out Optional output pointer for created descriptor.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t disk_create_new(
    DISK_TYPE type,
    uint64_t starting_lba,
    uint64_t ending_lba,
    int sector_size,
    struct disk** disk_out
);

/**
 * @return Primary physical disk descriptor, or NULL if not initialized.
 */
struct disk* disk_primary();

/**
 * @return Disk resolved as primary filesystem disk, or NULL.
 */
struct disk* disk_primary_fs_disk();

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

/**
 * Write one or more sectors from buffer to disk.
 *
 * @param idisk Disk descriptor.
 * @param lba Starting logical block address.
 * @param total Number of sectors to write.
 * @param buffer Input buffer.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t disk_write_block(struct disk* idisk, int lba, int total, const void* buffer);

#endif /* DISK_H */
