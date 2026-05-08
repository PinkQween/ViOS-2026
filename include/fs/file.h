#ifndef FILE_H
#define FILE_H

#include "fs/pparser.h"
#include "status.h"

/** File seek mode for file position operations. */
typedef unsigned int FILE_SEEK_MODE;
enum {
    /** Set absolute position from file start. */
    FILE_SEEK_SET = 0,
    /** Move position relative to current offset. */
    FILE_SEEK_CUR = 1,
    /** Move position relative to file end. */
    FILE_SEEK_END = 2
};

/** File open mode. */
typedef unsigned int FILE_MODE;
enum {
    /** Open for reading. */
    FILE_MODE_READ,
    /** Open for writing. */
    FILE_MODE_WRITE,
    /** Open for append writes. */
    FILE_MODE_APPEND,
    /** Invalid/unsupported mode marker. */
    FILE_MODE_INVALID
};

enum {
    /** File is read-only and cannot be written to. */
    FILE_STAT_READ_ONLY = 0b00000001
};

typedef unsigned int FILE_STAT_FLAGS;

struct disk;

/** File status information structure. */
struct file_stat {
    /* File type and permission flags. */
    FILE_STAT_FLAGS flags;
    /* File size in bytes. */
    uint32_t size;
};

/** Filesystem-specific open callback used by VFS. */
typedef void*(*FS_OPEN_FUNCTION)(struct disk* disk, struct path_root* path, FILE_SEEK_MODE mode);

/** Filesystem probe callback used to resolve disk format. */
typedef status_t(*FS_RESOLVE_FUNCTION)(struct disk* disk);

/** Filesystem-specific seek callback used by VFS. */
typedef status_t(*FS_SEEK_FUNCTION)(void* internal, uint32_t offset, FILE_SEEK_MODE whence);

/** Filesystem-specific stat callback used by VFS. */
typedef status_t(*FS_STAT_FUNCTION)(struct disk* disk, void* internal, struct file_stat* stat);

/** Filesystem-specific read callback used by VFS. */
typedef status_t(*FS_READ_FUNCTION)(struct disk* disk, void* fd, uint32_t size, uint32_t nmemb, char* buffer);

/** Filesystem-specific close callback used by VFS. */
typedef status_t(*FS_CLOSE_FUNCTION)(void* internal);

/** Registered filesystem driver descriptor. */
struct filesystem {
    /** Probe/resolve callback for this filesystem. */
    FS_RESOLVE_FUNCTION resolve;
    /** Open callback for this filesystem. */
    FS_OPEN_FUNCTION open;
    /** Seek callback for this filesystem. */
    FS_SEEK_FUNCTION seek;
    /** Read callback for this filesystem. */
    FS_READ_FUNCTION read;
    /** Stat callback for this filesystem. */
    FS_STAT_FUNCTION stat;
    /** Close callback for this filesystem. */
    FS_CLOSE_FUNCTION close;

    /** Human-readable filesystem name. */
    char name[20];
};

/** Runtime file descriptor owned by VFS. */
struct file_descriptor {
    /** 1-based descriptor id exposed to callers. */
    int index;
    /** Filesystem implementation backing this descriptor. */
    struct filesystem* fs;
    /** Filesystem-private descriptor payload. */
    void* internal;

    /** Disk associated with the open file. */
    struct disk* disk;
};

/**
 * Initialize the virtual filesystem registry and descriptor table.
 *
 * @return None.
 */
void fs_init();

/**
 * Open a file path with mode string.
 *
 * @param filepath Path like 0:/dir/file.txt.
 * @param mode Mode string (r, w, a).
 * @return Positive file descriptor id on success, negative status_t on error.
 */
status_t fopen(const char* filepath, const char* mode);

/**
 * Move file position for an open descriptor.
 * 
 * @param fd Open file descriptor id.
 * @param offset Byte offset to seek.
 * @param whence Seek mode (FILE_SEEK_*).
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t fseek(int fd, int offset, int whence);

/**
 * Read from an open file descriptor into a buffer.
 * 
 * @param ptr Output buffer for read data.
 * @param size Size of each element to read in bytes.
 * @param nmemb Number of elements to read.
 * @param fd Open file descriptor id.
 * @return Number of elements successfully read, or negative status_t on error.
 */
status_t fread(void* ptr, uint32_t size, uint32_t nmemb, int fd);

/**
 * Get status information for an open file descriptor.
 *
 * @param fd Open file descriptor id.
 * @param stat Output buffer for status information.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t fstat(int fd, struct file_stat* stat);

/**
 * Close an open file descriptor and free associated resources.
 * 
 * @param fd Open file descriptor id.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t fclose(int fd);

/**
 * Register a filesystem implementation in the global registry.
 *
 * @param fs Filesystem descriptor to register.
 * @return None.
 */
void fs_insert_filesystem(struct filesystem* fs);

/**
 * Resolve which filesystem can handle a given disk.
 *
 * @param disk Target disk descriptor.
 * @return Filesystem descriptor on success, NULL on failure.
 */
struct filesystem* fs_resolve(struct disk* disk);

#endif /* FILE_H */