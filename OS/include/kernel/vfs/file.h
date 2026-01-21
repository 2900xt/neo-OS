#pragma once
#include <stdlib/structures/string.h>

#include "drivers/disk/disk_driver.h"
#include <types.h>

#define FILE_READABLE (1 << 0)
#define FILE_WRITABLE (1 << 1)
#define FILE_EXECUTABLE (1 << 2)

namespace kernel
{

    struct file_handle
    {
        stdlib::string filename;
        size_t filesize;
        void *fat_entry;
        void *data;
        bool is_root;
        bool is_dir;
    };

    void vfs_init();
    void mount_root(disk::rw_disk_t *disk, uint64_t partition);
    file_handle *get_root();
    int open(file_handle *file, stdlib::string *filepath);
    void close(file_handle *file);
    void *read(file_handle *file);

}