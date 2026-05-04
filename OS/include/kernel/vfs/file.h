#pragma once
#include <stdlib/structures/string.h>

#include "drivers/disk/disk_driver.h"
#include <types.h>

namespace filesystem { struct FAT_partition; }

#define FILE_READABLE (1 << 0)
#define FILE_WRITABLE (1 << 1)
#define FILE_EXECUTABLE (1 << 2)

namespace kernel
{

    struct file_handle
    {
        stdlib::string filepath;
        size_t filesize;
        void *data, *file_entry;
        uint8_t attrib;
        uint64_t offset;
    };

    void vfs_init();
    void mount_root(disk::rw_disk_t *disk, uint64_t partition);
    file_handle *get_root();
    filesystem::FAT_partition *get_root_partition();
    int fopen(file_handle *file, stdlib::string *filepath);
    void close(file_handle *file);
}