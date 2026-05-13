#include "io/io.h"
#include "status.h"
#include "disk/disk.h"
#include "memory/memory.h"
#include "memory/heap/kheap.h"
#include "string/string.h"
#include "vector/vector.h"
#include "config.h"

#define ATA_DATA        0x1F0
#define ATA_SECCOUNT0   0x1F2
#define ATA_LBA0        0x1F3
#define ATA_LBA1        0x1F4
#define ATA_LBA2        0x1F5
#define ATA_HDDEVSEL    0x1F6
#define ATA_COMMAND     0x1F7
#define ATA_STATUS      0x1F7

#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30
#define ATA_CMD_FLUSH   0xE7

#define ATA_SR_BSY      0x80
#define ATA_SR_DRQ      0x08
#define ATA_SR_ERR      0x01

struct vector* disk_vector = NULL;

struct disk* disk = NULL;

struct disk* primary_fs_disk = NULL;

/**
 * Wait until BSY clears. Returns last status.
 */
static unsigned char disk_wait_busy()
{
    unsigned char status;
    
    do {
        status = inb(ATA_STATUS);
    } while (status & ATA_SR_BSY);

    return status;
}

/**
 * Wait until DRQ is set (data ready), with error check.
 */
static int disk_wait_drq()
{
    unsigned char status;

    while (1)
    {
        status = inb(ATA_STATUS);

        if (status & ATA_SR_ERR)
            return STATUS_ERR(EIO);

        if (!(status & ATA_SR_BSY) && (status & ATA_SR_DRQ))
            return STATUS_OK;
    }
}

/**
 * Select drive + LBA
 */
static void disk_select_lba(int lba, int total)
{
    outb(ATA_HDDEVSEL, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECCOUNT0, total);
    outb(ATA_LBA0, (unsigned char)(lba & 0xFF));
    outb(ATA_LBA1, (unsigned char)((lba >> 8) & 0xFF));
    outb(ATA_LBA2, (unsigned char)((lba >> 16) & 0xFF));
}

/**
 * Read sectors (PIO)
 */
static int disk_read_sector(int lba, int total, void* buffer)
{
    if (lba < 0 || total <= 0 || total > 0xFF || !buffer)
        return STATUS_ERR(EINVAL);

    unsigned short* ptr = (unsigned short*)buffer;

    disk_wait_busy();
    disk_select_lba(lba, total);
    outb(ATA_COMMAND, ATA_CMD_READ);

    for (int b = 0; b < total; b++)
    {
        if (disk_wait_drq() < 0)
            return STATUS_ERR(EIO);

        for (int i = 0; i < 256; i++)
        {
            *ptr++ = inw(ATA_DATA);
        }
    }

    return STATUS_OK;
}

/**
 * Write sectors (PIO)
 */
static int disk_write_sector(int lba, int total, const void* buffer)
{
    if (lba < 0 || total <= 0 || total > 0xFF || !buffer)
        return STATUS_ERR(EINVAL);

    const unsigned short* ptr = (const unsigned short*)buffer;

    disk_wait_busy();
    disk_select_lba(lba, total);
    outb(ATA_COMMAND, ATA_CMD_WRITE);

    for (int b = 0; b < total; b++)
    {
        if (disk_wait_drq() < 0)
            return STATUS_ERR(EIO);

        for (int i = 0; i < 256; i++)
        {
            outw(ATA_DATA, *ptr++);
        }
    }

    // Flush cache (important on real hardware)
    outb(ATA_COMMAND, ATA_CMD_FLUSH);
    disk_wait_busy();

    return STATUS_OK;
}

status_t disk_create_new(
    DISK_TYPE type,
    uint64_t starting_lba,
    uint64_t ending_lba,
    int sector_size,
    struct disk** disk_out
)
{
    if (!disk_vector || sector_size <= 0)
        return STATUS_ERR(EINVAL);

    struct disk* new_disk = kzalloc(sizeof(struct disk));
    if (!new_disk)
        return STATUS_ERR(ENOMEM);

    new_disk->type = type;
    new_disk->id = (int)vector_count(disk_vector);
    new_disk->sector_size = sector_size;
    new_disk->starting_lba = starting_lba;
    new_disk->ending_lba = ending_lba;

    status_t push_res = vector_push(disk_vector, &new_disk);
    if (status_is_error(push_res))
    {
        kfree(new_disk);
        return push_res;
    }

    new_disk->fs = fs_resolve(new_disk);

    if (new_disk->fs && !primary_fs_disk)
    {
        // First disk that resolves a filesystem becomes the primary fs disk.
        primary_fs_disk = new_disk;
    }

    if (disk_out)
        *disk_out = new_disk;

    return STATUS_OK;
}

status_t disk_search_and_init()
{
    if (disk_vector)
        return STATUS_OK;

    disk_vector = vector_new(sizeof(struct disk*), 4, VECTOR_NO_FLAGS);
    if (!disk_vector)
        return STATUS_ERR(ENOMEM);

    disk = NULL;
    primary_fs_disk = NULL;

    return disk_create_new(
        DISK_TYPE_REAL,
        0,
        0,
        SECTOR_SIZE_BYTES,
        &disk
    );
}

struct disk* disk_primary()
{
    return disk;
}

struct disk* disk_primary_fs_disk()
{
    return primary_fs_disk;
}

struct disk* disk_get(int index)
{
    if (!disk_vector || index < 0)
        return NULL;

    size_t total_disks = vector_count(disk_vector);
    if ((size_t)index >= total_disks)
        return NULL;

    struct disk* out = NULL;
    status_t res = vector_at(disk_vector, (size_t)index, &out, sizeof(out));
    if (status_is_error(res))
        return NULL;

    return out;
}

status_t disk_read_block(struct disk* idisk, int lba, int total, void* buffer)
{
    if (!idisk || lba < 0 || total <= 0 || !buffer)
        return STATUS_ERR(EINVAL);

    uint64_t absolute_lba = idisk->starting_lba + (uint64_t)lba;
    uint64_t absolute_end_lba = absolute_lba + (uint64_t)total - 1;

    if ((idisk->starting_lba != 0 || idisk->ending_lba != 0) &&
        absolute_end_lba > idisk->ending_lba)
    {
        return STATUS_ERR(EIO);
    }

    if (absolute_lba > 0x0FFFFFFFUL || absolute_end_lba > 0x0FFFFFFFUL)
        return STATUS_ERR(EINVAL);

    return disk_read_sector((int)absolute_lba, total, buffer);
}

status_t disk_write_block(struct disk* idisk, int lba, int total, const void* buffer)
{
    if (!idisk || lba < 0 || total <= 0 || !buffer)
        return STATUS_ERR(EINVAL);

    uint64_t absolute_lba = idisk->starting_lba + (uint64_t)lba;
    uint64_t absolute_end_lba = absolute_lba + (uint64_t)total - 1;

    if ((idisk->starting_lba != 0 || idisk->ending_lba != 0) &&
        absolute_end_lba > idisk->ending_lba)
    {
        return STATUS_ERR(EIO);
    }

    if (absolute_lba > 0x0FFFFFFFUL || absolute_end_lba > 0x0FFFFFFFUL)
        return STATUS_ERR(EINVAL);

    return disk_write_sector((int)absolute_lba, total, buffer);
}
