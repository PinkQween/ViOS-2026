#ifndef DISK_STREAMER_H
#define DISK_STREAMER_H

#include "disk/disk.h"

/** Sequential byte-oriented read helper over sector-based disk IO. */
struct disk_streamer {
    /** Current byte position in the underlying disk stream. */
    int pos;
    /** Disk being streamed. */
    struct disk* disk;
};

/**
 * Create a new streamer bound to disk index.
 *
 * @param disk_index Disk index to attach to.
 * @return New streamer on success, NULL on allocation/init failure.
 */
struct disk_streamer* disk_streamer_new(int disk_index);

/**
 * Set streamer position in bytes from disk start.
 *
 * @param streamer Streamer instance.
 * @param pos Absolute byte position.
 * @return STATUS_OK on success, negative status_t on error.
 */
int disk_streamer_seek(struct disk_streamer* streamer, int pos);

/**
 * Read raw bytes from streamer and advance internal position.
 *
 * @param streamer Streamer instance.
 * @param buffer Output buffer.
 * @param size Number of bytes to read.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t disk_streamer_read(struct disk_streamer* streamer, void* buffer, int size);

/**
 * Close and free streamer resources.
 *
 * @param streamer Streamer instance.
 * @return None.
 */
void disk_streamer_close(struct disk_streamer* streamer);

#endif /* DISK_STREAMER_H */