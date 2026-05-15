#include "fs/fat16/fat16_priv.h"

/* FAT chain traversal and cluster-backed reads. */

uint32_t fat16_get_first_cluster(struct fat_directory_entry* entry)
{
    return (entry->first_cluster_high << 16) | entry->first_cluster_low;
}

int fat16_cluster_to_sector(struct fat16_internal* fat_internal, int cluster)
{
    struct fat_header* header = &fat_internal->header.primary_header;
    int root_dir_bytes = header->root_entry_count * sizeof(struct fat_directory_entry);
    int root_dir_sectors = root_dir_bytes / header->bytes_per_sector;

    if (root_dir_bytes % header->bytes_per_sector) {
        root_dir_sectors++;
    }

    return fat_internal->header.primary_header.reserved_sectors
        + (fat_internal->header.primary_header.fat_count * fat_internal->header.primary_header.sectors_per_fat)
        + root_dir_sectors
        + ((cluster - 2) * fat_internal->header.primary_header.sectors_per_cluster);
}

static uint32_t fat16_get_first_fat_sector(struct fat16_internal* fat_internal)
{
    return fat_internal->header.primary_header.reserved_sectors;
}

int fat16_get_fat_entry(struct disk* disk, int cluster)
{
    struct fat16_internal* fat_internal = disk->fs_internal;
    struct disk_streamer* streamer = fat_internal->fat_streamer;

    if (!streamer) {
        return STATUS_ERR(ENOMEM);
    }

    uint32_t fat16_table_pos = fat16_get_first_fat_sector(fat_internal) * disk->sector_size;
    if (status_is_error(disk_streamer_seek(streamer, fat16_table_pos + (cluster * FAT16_FAT_ENTRY_SIZE)))) {
        return STATUS_ERR(EIO);
    }

    uint16_t result = 0;
    if (status_is_error(disk_streamer_read(streamer, &result, sizeof(result)))) {
        return STATUS_ERR(EIO);
    }

    return result;
}

int fat16_get_cluster_for_offset(struct disk* disk, struct fat16_internal* fat_internal, int cluster, int offset)
{
    int size_of_cluster = fat_internal->header.primary_header.sectors_per_cluster * disk->sector_size;
    int cluster_to_use = cluster;
    int cluster_offset = offset / size_of_cluster;

    for (int i = 0; i < cluster_offset; i++) {
        int entry = fat16_get_fat_entry(disk, cluster_to_use);

        if (entry == 0x00 || entry == FAT16_BAD_SECTOR || entry == 0xFF0 || entry == 0xFF6 || entry >= 0xFF8) {
            return STATUS_ERR(EIO);
        }

        cluster_to_use = entry;
    }

    return cluster_to_use;
}

static int fat16_read_from_streamer(struct disk* disk, struct disk_streamer* streamer, int cluster, int offset, int size, void* buffer)
{
    struct fat16_internal* fat_internal = disk->fs_internal;
    int size_of_cluster = fat_internal->header.primary_header.sectors_per_cluster * disk->sector_size;
    int bytes_read = 0;
    int current_offset = offset;
    int cluster_to_use = fat16_get_cluster_for_offset(disk, fat_internal, cluster, offset);

    if (cluster_to_use < 0) {
        return STATUS_ERR(EIO);
    }

    while (bytes_read < size) {
        int offset_from_cluster = current_offset % size_of_cluster;
        int starting_sector = fat16_cluster_to_sector(fat_internal, cluster_to_use);
        int starting_pos = fat16_sector_to_absolute(disk, starting_sector) + offset_from_cluster;
        int total_to_read = size - bytes_read;

        if (total_to_read > size_of_cluster - offset_from_cluster) {
            total_to_read = size_of_cluster - offset_from_cluster;
        }

        if (status_is_error(disk_streamer_seek(streamer, starting_pos))) {
            return STATUS_ERR(EIO);
        }

        if (status_is_error(disk_streamer_read(streamer, (uint8_t*)buffer + bytes_read, total_to_read))) {
            return STATUS_ERR(EIO);
        }

        bytes_read += total_to_read;
        current_offset += total_to_read;

        if (bytes_read < size && offset_from_cluster + total_to_read >= size_of_cluster) {
            int next_cluster = fat16_get_fat_entry(disk, cluster_to_use);

            if (next_cluster == 0x00 || next_cluster == FAT16_BAD_SECTOR || next_cluster == 0xFF0 || next_cluster == 0xFF6 || next_cluster >= 0xFF8) {
                return STATUS_ERR(EIO);
            }

            cluster_to_use = next_cluster;
        }
    }

    return STATUS_OK;
}

int fat16_read_internal(struct disk* disk, int cluster, int offset, int size, void* buffer)
{
    struct fat16_internal* fat_internal = disk->fs_internal;
    return fat16_read_from_streamer(disk, fat_internal->cluster_streamer, cluster, offset, size, buffer);
}
