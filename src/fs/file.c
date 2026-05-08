#include "fs/file.h"
#include "config.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "console/console.h"
#include "fs/fat16/fat16.h"
#include "disk/disk.h"
#include "string/string.h"

struct filesystem* filesystems[MAX_FILESYSTEMS];
struct file_descriptor* file_descriptors[MAX_FILE_DESCRIPTORS];

static struct filesystem** fs_get_free_filesystem_slot()
{
    for (int i = 0; i < MAX_FILESYSTEMS; i++)
    {
        if (!filesystems[i])
        {
            return &filesystems[i];
        }
    }

    return 0;
}

void fs_insert_filesystem(struct filesystem* fs)
{
    struct filesystem** slot = fs_get_free_filesystem_slot();

    if (!slot)
    {
        panic("No more space for filesystems");
    }

    *slot = fs;
}

static void fs_static_load()
{
    fs_insert_filesystem(fat16_init());
}

void fs_load()
{
    memset(filesystems, 0, sizeof(filesystems));
    fs_static_load();
}

void fs_init()
{
    memset(file_descriptors, 0, sizeof(file_descriptors));
    fs_load();
}

void file_free_descriptor(struct file_descriptor* descriptor)
{
    file_descriptors[descriptor->index - 1] = NULL;
    kfree(descriptor);
}

status_t file_new_descriptor(struct file_descriptor** descriptor_out)
{
    int res = STATUS_ERR(ENOSPC);

    for (int i = 0; i < MAX_FILE_DESCRIPTORS; i++)
    {
        if (!file_descriptors[i])
        {
            struct file_descriptor* fd = kmalloc(sizeof(struct file_descriptor));
            if (!fd) {
                return STATUS_ERR(ENOMEM);
            }

            fd->index = i + 1;
            file_descriptors[i] = fd;
            *descriptor_out = fd;
            res = STATUS_OK;
            break;
        }
    }

    return res;
}

static struct file_descriptor* file_get_descriptor(int fd)
{
    if (fd <= 0 || fd > MAX_FILE_DESCRIPTORS)
    {
        return NULL;
    }

    return file_descriptors[fd - 1];
}

struct filesystem* fs_resolve(struct disk* disk)
{
    struct filesystem* fs = NULL;

    for (int i = 0; i < MAX_FILESYSTEMS; i++)
    {
        if (!filesystems[i])
        {
            continue;
        }

        if (status_is_ok(filesystems[i]->resolve(disk)))
        {
            fs = filesystems[i];
            break;
        }
    }

    return fs;
}

FILE_MODE file_mode_from_string(const char* mode_str)
{
    if (strcmp(mode_str, "r") == 0 || strcmp(mode_str, "rb") == 0)
    {
        return FILE_MODE_READ;
    }
    else if (strcmp(mode_str, "w") == 0 || strcmp(mode_str, "wb") == 0)
    {
        return FILE_MODE_WRITE;
    }
    else if (strcmp(mode_str, "a") == 0 || strcmp(mode_str, "ab") == 0)
    {
        return FILE_MODE_APPEND;
    }
    else
    {
        return FILE_MODE_INVALID;
    }
}

status_t fopen(const char* filepath, const char* mode)
{
    int res = STATUS_OK;

    struct path_root* root = pathparser_parse(filepath, NULL);

    if (!root) {
        return STATUS_ERR(EINVAL);
    }

    if (!root->first) {
        pathparser_free(root);
        return STATUS_ERR(EINVAL);
    }

    struct disk* disk = disk_get(root->drive_number);

    if (!disk) {
        pathparser_free(root);
        return STATUS_ERR(ENOENT);
    }

    if (!disk->fs) {
        pathparser_free(root);
        return STATUS_ERR(ENOENT);
    }

    FILE_MODE file_mode = file_mode_from_string(mode);

    if (file_mode == FILE_MODE_INVALID) {
        pathparser_free(root);
        return STATUS_ERR(EINVAL);
    }

    void* descriptor_internal = disk->fs->open(disk, root, file_mode);

    status_t open_status = (status_t)descriptor_internal;
    if (status_is_error(open_status)) {
        pathparser_free(root);
        return open_status;
    }

    struct file_descriptor* fd = NULL;
    res = file_new_descriptor(&fd);

    if (status_is_error(res)) {
        if (disk->fs->close) {
            disk->fs->close(descriptor_internal);
        }
        pathparser_free(root);
        return res;
    }

    fd->fs = disk->fs;
    fd->internal = descriptor_internal;
    fd->disk = disk;
    res = fd->index;

    pathparser_free(root);
    
    return res;
}

status_t fseek(int fd, int offset, int whence)
{
    int res = STATUS_OK;

    struct file_descriptor* descriptor = file_get_descriptor(fd);

    if (!descriptor) {
        return STATUS_ERR(EINVAL);
    }

    if (!descriptor->fs || !descriptor->fs->seek) {
        return STATUS_ERR(EINVAL);
    }

    res = descriptor->fs->seek(descriptor->internal, offset, whence);
 
    return res;
}

status_t fread(void* buffer, uint32_t size, uint32_t nmemb, int fd)
{
    status_t res = STATUS_OK;

    if (size == 0 || nmemb == 0 || fd <= 0) {
        return STATUS_ERR(EINVAL);
    }

    struct file_descriptor* descriptor = file_get_descriptor(fd);

    if (!descriptor) {
        return STATUS_ERR(EINVAL);
    }

    if (!descriptor->fs || !descriptor->fs->read) {
        return STATUS_ERR(EINVAL);
    }

    return descriptor->fs->read(descriptor->disk, descriptor->internal, size, nmemb, buffer);
}

status_t fstat(int fd, struct file_stat* stat)
{
    status_t res = STATUS_OK;

    if (fd <= 0 || !stat) {
        return STATUS_ERR(EINVAL);
    }

    struct file_descriptor* descriptor = file_get_descriptor(fd);

    if (!descriptor) {
        return STATUS_ERR(EINVAL);
    }

    if (!descriptor->fs || !descriptor->fs->stat) {
        return STATUS_ERR(EINVAL);
    }

    res = descriptor->fs->stat(descriptor->disk, descriptor->internal, stat);

    return res;
}

status_t fclose(int fd)
{
    status_t res = STATUS_OK;

    if (fd <= 0) {
        return STATUS_ERR(EINVAL);
    }

    struct file_descriptor* descriptor = file_get_descriptor(fd);

    if (!descriptor) {
        return STATUS_ERR(EINVAL);
    }

    if (descriptor->fs && descriptor->fs->close) {
        res = descriptor->fs->close(descriptor->internal);
    }

    if (status_is_error(res)) {
        return res;
    }

    file_free_descriptor(descriptor);

    return res;
}
