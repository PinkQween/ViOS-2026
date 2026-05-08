#include "io/io.h"
#include "status.h"
#include "disk/disk.h"
#include "memory/memory.h"
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

struct disk disk;

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

status_t disk_search_and_init()
{
    memset(&disk, 0, sizeof(disk));
    disk.type = DISK_TYPE_REAL;
    disk.sector_size = SECTOR_SIZE_BYTES;
    disk.id = 0;
    disk.fs = fs_resolve(&disk);

    if (!disk.fs) {
        return STATUS_ERR(ENODEV);
    }

    return STATUS_OK;
}

struct disk* disk_get(int index)
{
    if (index != 0)
        return 0;

    return &disk;
}

status_t disk_read_block(struct disk* idisk, int lba, int total, void* buffer)
{
    if (idisk != &disk || lba < 0 || total <= 0 || !buffer)
        return STATUS_ERR(EINVAL);

    return disk_read_sector(lba, total, buffer);
}

status_t disk_write_block(struct disk* idisk, int lba, int total, const void* buffer)
{
    if (idisk != &disk || lba < 0 || total <= 0 || !buffer)
        return STATUS_ERR(EINVAL);

    return disk_write_sector(lba, total, buffer);
}
