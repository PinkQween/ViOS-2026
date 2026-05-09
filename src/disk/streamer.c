#include "disk/streamer.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "config.h"
#include "status.h"

#include "stdbool.h"

struct disk_streamer* disk_streamer_new(int disk_index) {
    struct disk* disk = disk_get(disk_index);

    if (!disk) {
        return NULL;
    }

    struct disk_streamer* streamer = kzalloc(sizeof(struct disk_streamer));
    if (!streamer) {
        return NULL;
    }

    streamer->pos = 0;
    streamer->disk = disk;

    return streamer;
}

int disk_streamer_seek(struct disk_streamer* streamer, int pos) {
    if (!streamer || pos < 0) {
        return STATUS_ERR(EINVAL);
    }

    streamer->pos = pos;

    return STATUS_OK;
}

status_t disk_streamer_read(struct disk_streamer* streamer, void* buffer, int size) {
    if (!streamer || !buffer || size < 0) {
        return STATUS_ERR(EINVAL);
    }
    
    if (size == 0) {
        return STATUS_OK;
    }

    int bytes_read = 0;

    while (bytes_read < size) {
        int sector = streamer->pos / SECTOR_SIZE_BYTES;
        int offset = streamer->pos % SECTOR_SIZE_BYTES;
        char buf[SECTOR_SIZE_BYTES];

        int res = disk_read_block(streamer->disk, sector, 1, buf);
        
        if (status_is_error(res)) {
            return res;
        }
        
        int to_copy = SECTOR_SIZE_BYTES - offset;
        
        if (to_copy > (size - bytes_read)) {
            to_copy = size - bytes_read;
        }

        memcpy((char*)buffer + bytes_read, buf + offset, to_copy);
        bytes_read += to_copy;
        streamer->pos += to_copy;
    }

    return STATUS_OK;
}

void disk_streamer_close(struct disk_streamer* stream)
{
    kfree(stream);
}
