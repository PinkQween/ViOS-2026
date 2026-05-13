#include "fs/fat16/fat16_priv.h"

/* Top-level FAT16 filesystem wiring (init/resolve/open). */

struct filesystem fat16_fs =
{
    .resolve = fat16_resolve,
    .open = fat16_open,
    .write = fat16_write,
    .read = fat16_read,
    .seek = fat16_seek,
    .stat = fat16_stat,
    .close = fat16_close,
    .volume_name = fat16_volume_name
};

struct filesystem* fat16_init()
{
    strcpy(fat16_fs.name, "FAT16");
    return &fat16_fs;
}

static void fat16_init_private(struct disk* disk, struct fat16_internal* fat_internal)
{
    memset(fat_internal, 0, sizeof(struct fat16_internal));
    fat_internal->cluster_streamer = disk_streamer_new(disk->id);
    fat_internal->fat_streamer = disk_streamer_new(disk->id);
    fat_internal->directory_streamer = disk_streamer_new(disk->id);
}

status_t fat16_resolve(struct disk* disk)
{
    status_t res = STATUS_OK;
    struct disk_streamer* streamer = NULL;

    struct fat16_internal* fat_internal = kmalloc(sizeof(struct fat16_internal));
    
    if (!fat_internal) {
        return STATUS_ERR(ENOMEM);
    }

    fat16_init_private(disk, fat_internal);
    if (!fat_internal->cluster_streamer || !fat_internal->fat_streamer || !fat_internal->directory_streamer) {
        res = STATUS_ERR(ENOMEM);
        goto out;
    }

    disk->fs_internal = fat_internal;
    disk->fs = &fat16_fs;

    streamer = disk_streamer_new(disk->id);

    if (!streamer) {
        res = STATUS_ERR(ENOMEM);
        goto out;
    }

    res = disk_streamer_read(streamer, &fat_internal->header.primary_header, sizeof(struct fat_header));
    if (status_is_error(res)) {
        goto out;
    }

    if (fat_internal->header.primary_header.extended.signature != FAT16_SIGNATURE) {
        res = STATUS_ERR(ENODEV);
        goto out;
    }

    res = fat16_get_root_directory(disk, fat_internal, &fat_internal->root_directory);

    if (status_is_error(res)) {
        goto out;
    }

    strncpy(fat_internal->name, (const char*) fat_internal->header.shared.extended_header.volume_id_string, sizeof(fat_internal->name));

out:
    if (streamer) {
        disk_streamer_close(streamer);
    }

    if (status_is_error(res)) {
        if (fat_internal->cluster_streamer) {
            disk_streamer_close(fat_internal->cluster_streamer);
        }
        if (fat_internal->fat_streamer) {
            disk_streamer_close(fat_internal->fat_streamer);
        }
        if (fat_internal->directory_streamer) {
            disk_streamer_close(fat_internal->directory_streamer);
        }
        kfree(fat_internal);
        disk->fs_internal = NULL;
        disk->fs = NULL;
    }

    return res;
}

void* fat16_open(struct disk* disk, struct path_root* path, FILE_SEEK_MODE mode)
{
    if (!path || !path->first) {
        return (void*)STATUS_ERR(EINVAL);
    }

    struct fat16_file_descriptor* fd = kmalloc(sizeof(struct fat16_file_descriptor));
    
    if (!fd) {
        return (void*)STATUS_ERR(ENOMEM);
    }

    struct fat16_item* descriptor_item = fat16_get_directory_entry(disk, path->first);

    if (!descriptor_item) {
        kfree(fd);
        return (void*)STATUS_ERR(ENOENT);
    }

    fd->item = *descriptor_item;
    kfree(descriptor_item);

    fd->mode = mode;
    fd->pos = 0;

    if (mode == FILE_MODE_APPEND && fd->item.type == FAT_ITEM_TYPE_FILE) {
        fd->pos = fd->item.entry->file_size;
    }

    return fd;
}

status_t fat16_seek(void* internal, uint32_t offset, FILE_SEEK_MODE whence)
{
    struct fat16_file_descriptor* fd = internal;

    if (!fd) {
        return STATUS_ERR(EINVAL);
    }

    if (fd->item.type != FAT_ITEM_TYPE_FILE) {
        return STATUS_ERR(EINVAL);
    }

    struct fat_directory_entry* entry = fd->item.entry;
    
    if (offset > entry->file_size) {
        return STATUS_ERR(EINVAL);
    }

    switch (whence) {
        case FILE_SEEK_SET:
            fd->pos = offset;
            break;
        case FILE_SEEK_CUR:
            fd->pos += offset;
            break;
        case FILE_SEEK_END:
            fd->pos = entry->file_size + offset;
            break;
        default:
            return STATUS_ERR(EINVAL);
    }

    if (fd->pos > entry->file_size) {
        return STATUS_ERR(EINVAL);
    }

    return STATUS_OK;
}

status_t fat16_read(struct disk* disk, void* fd, uint32_t size, uint32_t nmemb, char* buffer)
{
    struct fat16_file_descriptor* fat_fd = fd;
    struct fat_directory_entry* entry = fat_fd->item.entry;
    uint32_t total = size * nmemb;

    if (!entry || !buffer) {
        return STATUS_ERR(EINVAL);
    }

    if (total == 0) {
        return 0;
    }

    if (fat_fd->pos > entry->file_size || total > entry->file_size - fat_fd->pos) {
        return STATUS_ERR(EIO);
    }

    status_t res = fat16_read_internal(disk, fat16_get_first_cluster(entry), fat_fd->pos, total, buffer);
    if (status_is_error(res)) {
        return res;
    }

    fat_fd->pos += total;
    return nmemb;
}

static status_t fat16_write_sector(
    struct disk* disk,
    uint32_t sector_index,
    uint32_t sector_offset,
    const char* buffer,
    uint32_t bytes_to_write,
    uint8_t* sector_buffer
)
{
    status_t res = disk_read_block(disk, (int)sector_index, 1, sector_buffer);
    if (status_is_error(res)) {
        return res;
    }

    memcpy(sector_buffer + sector_offset, buffer, bytes_to_write);

    return disk_write_block(disk, (int)sector_index, 1, sector_buffer);
}

status_t fat16_write(struct disk* disk, void* fd, uint32_t size, uint32_t nmemb, const char* buffer)
{
    struct fat16_file_descriptor* fat_fd = fd;
    struct fat_directory_entry* entry = fat_fd ? fat_fd->item.entry : NULL;
    uint32_t total = size * nmemb;

    if (!disk || !fat_fd || !entry || !buffer) {
        return STATUS_ERR(EINVAL);
    }

    if (fat_fd->item.type != FAT_ITEM_TYPE_FILE) {
        return STATUS_ERR(EINVAL);
    }

    if (entry->attributes & FAT_FILE_READ_ONLY) {
        return STATUS_ERR(EACCES);
    }

    if (fat_fd->mode == FILE_MODE_READ) {
        return STATUS_ERR(EACCES);
    }

    if (total == 0) {
        return 0;
    }

    if (fat_fd->pos > entry->file_size || total > entry->file_size - fat_fd->pos) {
        return STATUS_ERR(EIO);
    }

    struct fat16_internal* fat_internal = disk->fs_internal;
    if (!fat_internal) {
        return STATUS_ERR(ENODEV);
    }

    uint32_t first_cluster = fat16_get_first_cluster(entry);
    uint32_t bytes_written = 0;

    while (bytes_written < total) {
        int cluster = fat16_get_cluster_for_offset(disk, fat_internal, (int)first_cluster, (int)fat_fd->pos);
        if (cluster < 0) {
            return STATUS_ERR(EIO);
        }

        int cluster_size = fat_internal->header.primary_header.sectors_per_cluster * disk->sector_size;
        int offset_in_cluster = (int)(fat_fd->pos % (uint32_t)cluster_size);
        int starting_sector = fat16_cluster_to_sector(fat_internal, cluster);
        int absolute_byte = fat16_sector_to_absolute(disk, starting_sector) + offset_in_cluster;
        uint32_t sector_index = (uint32_t)(absolute_byte / disk->sector_size);
        uint32_t sector_offset = (uint32_t)(absolute_byte % disk->sector_size);
        uint32_t bytes_available_in_sector = (uint32_t)(disk->sector_size - sector_offset);
        uint32_t bytes_remaining = total - bytes_written;
        uint32_t bytes_to_write = bytes_available_in_sector < bytes_remaining ? bytes_available_in_sector : bytes_remaining;
        uint8_t sector_buffer[SECTOR_SIZE_BYTES];

        status_t res = fat16_write_sector(
            disk,
            sector_index,
            sector_offset,
            buffer + bytes_written,
            bytes_to_write,
            sector_buffer
        );

        if (status_is_error(res)) {
            return res;
        }

        bytes_written += bytes_to_write;
        fat_fd->pos += bytes_to_write;
    }

    return nmemb;
}

void fat16_free_file_descriptor(struct fat16_file_descriptor* internal)
{
    if (!internal) {
        return;
    }

    if (internal->item.type == FAT_ITEM_TYPE_DIRECTORY) {
        fat16_free_directory(internal->item.directory);
    } else if (internal->item.type == FAT_ITEM_TYPE_FILE) {
        kfree(internal->item.entry);
    }

    kfree(internal);
}

status_t fat16_stat(struct disk* disk, void* fd, struct file_stat* stat)
{
    (void)disk;

    if (!fd || !stat) {
        return STATUS_ERR(EINVAL);
    }

    struct fat16_file_descriptor* fat_fd = fd;
    struct fat_directory_entry* entry = fat_fd->item.entry;

    if (!entry) {
        return STATUS_ERR(EINVAL);
    }

    stat->size = entry->file_size;
    stat->flags = (entry->attributes & FAT_FILE_READ_ONLY) ? FILE_STAT_READ_ONLY : 0;

    return STATUS_OK;
}

status_t fat16_close(void* internal)
{
    if (!internal) {
        return STATUS_ERR(EINVAL);
    }

    fat16_free_file_descriptor((struct fat16_file_descriptor*)internal);

    return STATUS_OK;
}

status_t fat16_volume_name(void* internal, char* name_out, size_t max)
{
    if (!internal || !name_out || max == 0) {
        return STATUS_ERR(EINVAL);
    }

    struct fat16_internal* fat_internal = internal;
    
    strncpy(name_out, fat_internal->name, max);

    return STATUS_OK;
}