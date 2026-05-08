#include "fs/fat16/fat16_priv.h"

/* Top-level FAT16 filesystem wiring (init/resolve/open). */

struct filesystem fat16_fs =
{
    .resolve = fat16_resolve,
    .open = fat16_open,
    .read = fat16_read,
    .seek = fat16_seek,
    .stat = fat16_stat,
    .close = fat16_close
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
    if (mode != FILE_MODE_READ) {
        return (void*)STATUS_ERR(EACCES);
    }

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

    fd->pos = 0;

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
    
    if (offset >= entry->file_size) {
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
    int res = STATUS_OK;

    struct fat16_file_descriptor* fat_fd = fd;
    struct fat_directory_entry* entry = fat_fd->item.entry;

    int offset = fat_fd->pos;
    char* original_buffer = buffer;

    for (uint32_t i = 0; i < nmemb; i++) {
        if (status_is_error(res = fat16_read_internal(disk, fat16_get_first_cluster(entry), offset, size, original_buffer + (i * size)))) {
            return res;
        }
    
        offset += size;
    }

    res = nmemb;

    return res;
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
