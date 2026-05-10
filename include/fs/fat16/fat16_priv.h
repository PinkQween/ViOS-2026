#ifndef FAT16_PRIV_H
#define FAT16_PRIV_H

/*
 * Copyright (c) 2026 Hanna Skairipa.
 */

/**
 * @file fat16_priv.h
 * @brief Internal FAT16 filesystem structures and helpers.
 *
 * @author Hanna Skairipa
 * @date 2026-05-09
 */

#include "fs/fat16/fat16.h"
#include "status.h"
#include "config.h"
#include "string/string.h"
#include "memory/memory.h"
#include "disk/disk.h"
#include "disk/streamer.h"
#include "memory/heap/kheap.h"

#define FAT16_SIGNATURE 0x29
#define FAT16_FAT_ENTRY_SIZE 0x02
#define FAT16_BAD_SECTOR 0xFF7

typedef unsigned int FAT_ITEM_TYPE;

#define FAT_ITEM_TYPE_DIRECTORY 0
#define FAT_ITEM_TYPE_FILE 1

#define FAT_FILE_READ_ONLY 0x01
#define FAT_FILE_SUBDIRECTORY 0x10

struct fat_header_extended {
    uint8_t drive_number;
    uint8_t win_nt_bit;
    uint8_t signature;
    uint32_t volume_id;
    uint8_t volume_id_string[11];
    uint8_t fat_type_label[8];
} __attribute__((packed));

struct fat_header
{
    uint8_t jump_instruction[3];
    uint8_t oem_name[8];
    uint16_t bytes_per_sector;
    uint8_t sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t fat_count;
    uint16_t root_entry_count;
    uint16_t total_sectors_short;
    uint8_t media_descriptor;
    uint16_t sectors_per_fat;
    uint16_t sectors_per_track;
    uint16_t head_side_count;
    uint32_t hidden_sector_count;
    uint32_t total_sectors_long;

    struct fat_header_extended extended;
} __attribute__((packed));

struct fat_h
{
    struct fat_header primary_header;
    union fat_h_e
    {
        struct fat_header_extended extended_header;
    } shared;
};

struct fat_directory_entry
{
    uint8_t name[8];
    uint8_t extension[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t creation_time_tenths;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t last_modification_time;
    uint16_t last_modification_date;
    uint16_t first_cluster_low;
    uint32_t file_size;
} __attribute__((packed));

struct fat16_directory
{
    struct fat_directory_entry* entries;
    size_t entry_count;
    int sectors_pos;
    int ending_sector_pos;
};

struct fat16_item
{
    union {
        struct fat_directory_entry* entry;
        struct fat16_directory* directory;
    };

    FAT_ITEM_TYPE type;
};

struct fat16_file_descriptor
{
    struct fat16_item item;
    uint32_t pos;
};

struct fat16_internal
{
    struct fat_h header;
    struct fat16_directory root_directory;

    struct disk_streamer* cluster_streamer;
    struct disk_streamer* fat_streamer;
    struct disk_streamer* directory_streamer;
};

extern struct filesystem fat16_fs;

/**
 * Resolve FAT16 on the given disk and initialize FAT16 runtime context.
 *
 * @param disk Target disk descriptor.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t fat16_resolve(struct disk* disk);

/**
 * Open a FAT16 file by parsed path in read mode.
 *
 * @param disk Target disk descriptor.
 * @param path Parsed path root.
 * @param mode Open mode.
 * @return Filesystem-private descriptor or encoded error pointer.
 */
void* fat16_open(struct disk* disk, struct path_root* path, FILE_SEEK_MODE mode);

/**
 * Seek within an open FAT16 file descriptor.
 *
 * @param internal FAT16 file descriptor pointer.
 * @param offset Byte offset to seek.
 * @param whence Seek mode (FILE_SEEK_*).
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t fat16_seek(void* internal, uint32_t offset, FILE_SEEK_MODE whence);

/**
 * Read from an open FAT16 file descriptor.
 *
 * @param disk Target disk descriptor.
 * @param fd FAT16 file descriptor pointer.
 * @param size Size of each element to read.
 * @param nmemb Number of elements to read.
 * @param buffer Output buffer for read data.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t fat16_read(struct disk* disk, void* fd, uint32_t size, uint32_t nmemb, char* buffer);

/**
 * Release a FAT16 file descriptor and any owned payload.
 *
 * @param internal FAT16 file descriptor pointer.
 * @return None.
 */
void fat16_free_file_descriptor(struct fat16_file_descriptor* internal);

/**
 * Get status information for an open FAT16 file descriptor.
 * 
 * @param disk Target disk descriptor.
 * @param fd FAT16 file descriptor pointer.
 * @param stat Output buffer for status information.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t fat16_stat(struct disk* disk, void* fd, struct file_stat* stat);

/**
 * Close and free a FAT16 file descriptor.
 *
 * @param internal FAT16-private file descriptor.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t fat16_close(void* internal);

/**
 * Convert a sector index to absolute byte offset on disk.
 *
 * @param disk Target disk descriptor.
 * @param sector Sector index.
 * @return Absolute byte offset.
 */
int fat16_sector_to_absolute(struct disk* disk, int sector);

/**
 * Count valid directory items until FAT end marker (0x00).
 *
 * @param disk Target disk descriptor.
 * @param directory_start_sector Directory start sector.
 * @return Item count on success, negative status_t on error.
 */
int fat16_get_total_items_for_directory(struct disk* disk, uint32_t directory_start_sector);

/**
 * Load root directory metadata and entries into FAT16 internal context.
 *
 * @param disk Target disk descriptor.
 * @param fat_internal FAT16 runtime context.
 * @param out_directory Output directory descriptor.
 * @return STATUS_OK on success, negative status_t on error.
 */
status_t fat16_get_root_directory(struct disk* disk, struct fat16_internal* fat_internal, struct fat16_directory* out_directory);

/**
 * Copy FAT 8.3 text field into normal null-terminated string form.
 *
 * @param dest Destination write cursor pointer.
 * @param src FAT field source string.
 * @return None.
 */
void fat16_to_proper_string(char** dest, const char* src);

/**
 * Build a normalized file name from FAT directory entry (NAME.EXT).
 *
 * @param entry FAT directory entry.
 * @param out Destination filename buffer.
 * @param max_len Output buffer length.
 * @return None.
 */
void fat16_get_full_relative_filename(struct fat_directory_entry* entry, char* out, int max_len);

/**
 * Clone a FAT directory entry into heap memory.
 *
 * @param entry Source FAT entry.
 * @param size Bytes to copy.
 * @return Cloned entry pointer or NULL on failure.
 */
struct fat_directory_entry* fat16_clone_directory_entry(struct fat_directory_entry* entry, int size);

/**
 * Read first data cluster index from FAT directory entry.
 *
 * @param entry FAT directory entry.
 * @return First cluster index.
 */
uint32_t fat16_get_first_cluster(struct fat_directory_entry* entry);

/**
 * Convert cluster index into data sector index.
 *
 * @param fat_internal FAT16 runtime context.
 * @param cluster Cluster number.
 * @return Data sector index.
 */
int fat16_cluster_to_sector(struct fat16_internal* fat_internal, int cluster);

/**
 * Read raw FAT table entry for a cluster.
 *
 * @param disk Target disk descriptor.
 * @param cluster Cluster number.
 * @return FAT table value or negative status_t on error.
 */
int fat16_get_fat_entry(struct disk* disk, int cluster);

/**
 * Resolve which cluster backs a given byte offset in a file stream.
 *
 * @param disk Target disk descriptor.
 * @param fat_internal FAT16 runtime context.
 * @param cluster First cluster in file chain.
 * @param offset Byte offset inside the file.
 * @return Cluster index or negative status_t on error.
 */
int fat16_get_cluster_for_offset(struct disk* disk, struct fat16_internal* fat_internal, int cluster, int offset);

/**
 * Read bytes from FAT16 file content using cluster traversal.
 *
 * @param disk Target disk descriptor.
 * @param cluster First file cluster.
 * @param offset Byte offset within file.
 * @param size Number of bytes to read.
 * @param buffer Output buffer.
 * @return STATUS_OK on success, negative status_t on error.
 */
int fat16_read_internal(struct disk* disk, int cluster, int offset, int size, void* buffer);

/**
 * Free directory object and owned entry storage.
 *
 * @param directory Directory object to free.
 * @return None.
 */
void fat16_free_directory(struct fat16_directory* directory);

/**
 * Free item wrapper and owned payload (file entry or directory).
 *
 * @param item Item wrapper to free.
 * @return None.
 */
void fat16_free_item(struct fat16_item* item);

/**
 * Load a subdirectory from disk for a directory entry.
 *
 * @param disk Target disk descriptor.
 * @param entry Directory entry describing a subdirectory.
 * @return Loaded directory object or NULL on failure.
 */
struct fat16_directory* fat16_load_fat_directory(struct disk* disk, struct fat_directory_entry* entry);

/**
 * Wrap a FAT directory entry as a typed runtime item.
 *
 * @param disk Target disk descriptor.
 * @param entry FAT directory entry.
 * @return FAT item wrapper or NULL on failure.
 */
struct fat16_item* fat16_new_item_for_entry(struct disk* disk, struct fat_directory_entry* entry);

/**
 * Find a child item by case-insensitive name in a directory.
 *
 * @param disk Target disk descriptor.
 * @param directory Directory to search.
 * @param name Name to match.
 * @return Matching item or NULL if not found.
 */
struct fat16_item* fat16_find_item_in_directory(struct disk* disk, struct fat16_directory* directory, const char* name);

/**
 * Resolve a parsed path chain into a FAT item.
 *
 * @param disk Target disk descriptor.
 * @param path Linked path components.
 * @return Resolved FAT item or NULL on failure.
 */
struct fat16_item* fat16_get_directory_entry(struct disk* disk, struct path_part* path);

#endif /* FAT16_PRIV_H */
