#include "fs/fat16/fat16_priv.h"

/* Directory and name handling helpers for FAT16. */

int fat16_sector_to_absolute(struct disk* disk, int sector)
{
    return sector * disk->sector_size;
}

int fat16_get_total_items_for_directory(struct disk* disk, uint32_t directory_start_sector)
{
    struct fat16_internal* fat_internal = disk->fs_internal;
    struct disk_streamer* streamer = fat_internal->directory_streamer;
    struct fat_directory_entry entry;
    int total_items = 0;

    if (status_is_error(disk_streamer_seek(streamer, fat16_sector_to_absolute(disk, directory_start_sector)))) {
        return STATUS_ERR(EIO);
    }

    while (true) {
        if (status_is_error(disk_streamer_read(streamer, &entry, sizeof(entry)))) {
            return STATUS_ERR(EIO);
        }

        if (entry.name[0] == 0x00) {
            break;
        }

        if (entry.name[0] == 0xE5) {
            continue;
        }

        total_items++;
    }

    return total_items;
}

status_t fat16_get_root_directory(struct disk* disk, struct fat16_internal* fat_internal, struct fat16_directory* out_directory)
{
    struct fat_header* header = &fat_internal->header.primary_header;
    int root_dir_sector_pos = (header->fat_count * header->sectors_per_fat) + header->reserved_sectors;
    int root_dir_entries = header->root_entry_count;
    int root_dir_size_bytes = root_dir_entries * sizeof(struct fat_directory_entry);
    int root_dir_sectors = root_dir_size_bytes / disk->sector_size;
    struct disk_streamer* streamer = fat_internal->directory_streamer;

    if (root_dir_size_bytes % disk->sector_size) {
        root_dir_sectors += 1;
    }

    int total_items = fat16_get_total_items_for_directory(disk, root_dir_sector_pos);
    if (status_is_error(total_items)) {
        return total_items;
    }

    struct fat_directory_entry* entries = kzalloc(root_dir_size_bytes);
    if (!entries) {
        return STATUS_ERR(ENOMEM);
    }

    if (status_is_error(disk_streamer_seek(streamer, fat16_sector_to_absolute(disk, root_dir_sector_pos)))) {
        kfree(entries);
        return STATUS_ERR(EIO);
    }

    if (status_is_error(disk_streamer_read(streamer, entries, root_dir_size_bytes))) {
        kfree(entries);
        return STATUS_ERR(EIO);
    }

    out_directory->entries = entries;
    out_directory->entry_count = total_items;
    out_directory->sectors_pos = root_dir_sector_pos;
    out_directory->ending_sector_pos = root_dir_sector_pos + root_dir_sectors;

    return STATUS_OK;
}

void fat16_to_proper_string(char** dest, const char* src)
{
    while (*src != '\0' && *src != ' ') {
        **dest = *src;
        (*dest)++;
        src++;
    }

    **dest = '\0';
}

void fat16_get_full_relative_filename(struct fat_directory_entry* entry, char* out, int max_len)
{
    memset(out, 0x00, max_len);
    char* out_ptr = out;

    fat16_to_proper_string(&out_ptr, (char*)entry->name);

    if (entry->extension[0] != ' ' && entry->extension[0] != '\0') {
        *out_ptr = '.';
        out_ptr++;
        fat16_to_proper_string(&out_ptr, (char*)entry->extension);
    }
}

struct fat_directory_entry* fat16_clone_directory_entry(struct fat_directory_entry* entry, int size)
{
    if (size < sizeof(struct fat_directory_entry)) {
        return NULL;
    }

    struct fat_directory_entry* clone = kzalloc(size);
    if (!clone) {
        return NULL;
    }

    memcpy(clone, entry, size);
    return clone;
}

void fat16_free_directory(struct fat16_directory* directory)
{
    if (!directory) {
        return;
    }

    if (directory->entries) {
        kfree(directory->entries);
    }

    kfree(directory);
}

void fat16_free_item(struct fat16_item* item)
{
    if (!item) {
        return;
    }

    if (item->type == FAT_ITEM_TYPE_DIRECTORY) {
        fat16_free_directory(item->directory);
    } else if (item->type == FAT_ITEM_TYPE_FILE) {
        kfree(item->entry);
    }

    kfree(item);
}

struct fat16_directory* fat16_load_fat_directory(struct disk* disk, struct fat_directory_entry* entry)
{
    if (!(entry->attributes & FAT_FILE_SUBDIRECTORY)) {
        return NULL;
    }

    struct fat16_internal* fat_internal = disk->fs_internal;
    struct fat16_directory* directory = kzalloc(sizeof(struct fat16_directory));
    if (!directory) {
        return NULL;
    }

    int cluster = fat16_get_first_cluster(entry);
    int cluster_sector = fat16_cluster_to_sector(fat_internal, cluster);
    int total_items = fat16_get_total_items_for_directory(disk, cluster_sector);
    if (status_is_error(total_items)) {
        fat16_free_directory(directory);
        return NULL;
    }

    int directory_size_bytes = total_items * sizeof(struct fat_directory_entry);
    directory->entry_count = total_items;
    directory->entries = kzalloc(directory_size_bytes);

    if (!directory->entries) {
        fat16_free_directory(directory);
        return NULL;
    }

    if (status_is_error(fat16_read_internal(disk, cluster, 0x00, directory_size_bytes, directory->entries))) {
        fat16_free_directory(directory);
        return NULL;
    }

    return directory;
}

struct fat16_item* fat16_new_item_for_entry(struct disk* disk, struct fat_directory_entry* entry)
{
    struct fat16_item* item = kzalloc(sizeof(struct fat16_item));
    if (!item) {
        return NULL;
    }

    if (entry->attributes & FAT_FILE_SUBDIRECTORY) {
        item->type = FAT_ITEM_TYPE_DIRECTORY;
        item->directory = fat16_load_fat_directory(disk, entry);

        if (!item->directory) {
            kfree(item);
            return NULL;
        }
    } else {
        item->type = FAT_ITEM_TYPE_FILE;
        item->entry = fat16_clone_directory_entry(entry, sizeof(struct fat_directory_entry));

        if (!item->entry) {
            kfree(item);
            return NULL;
        }
    }

    return item;
}

struct fat16_item* fat16_find_item_in_directory(struct disk* disk, struct fat16_directory* directory, const char* name)
{
    char tmp_filename[MAX_PATH_LENGTH];

    for (int i = 0; i < directory->entry_count; i++) {
        fat16_get_full_relative_filename(&directory->entries[i], tmp_filename, sizeof(tmp_filename));

        if (istrncmp(tmp_filename, name, sizeof(tmp_filename)) == 0) {
            return fat16_new_item_for_entry(disk, &directory->entries[i]);
        }
    }

    return NULL;
}

struct fat16_item* fat16_get_directory_entry(struct disk* disk, struct path_part* path)
{
    if (!path) {
        return NULL;
    }

    struct fat16_internal* fat_internal = disk->fs_internal;
    struct fat16_item* current_item = fat16_find_item_in_directory(disk, &fat_internal->root_directory, path->name);

    if (!current_item) {
        return NULL;
    }

    struct path_part* next_part = path->next;

    while (next_part) {
        if (current_item->type != FAT_ITEM_TYPE_DIRECTORY) {
            fat16_free_item(current_item);
            return NULL;
        }

        struct fat16_item* next_item = fat16_find_item_in_directory(disk, current_item->directory, next_part->name);
        fat16_free_directory(current_item->directory);
        current_item = next_item;

        if (!current_item) {
            return NULL;
        }

        next_part = next_part->next;
    }

    return current_item;
}
