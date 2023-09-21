#pragma once
#include <stdlib/string.h>
#include "drivers/disk/disk_driver.h"
#include <types.h>

#define FILE_READABLE       (1 << 0)
#define FILE_WRITABLE       (1 << 1)
#define FILE_EXECUTABLE     (1 << 2)

namespace VFS
{

struct File 
{
    std::string filename;
    size_t filesize;
    void* fat_entry;
};

void vfs_init();
void mount_root(DISK::rw_disk_t *disk, uint64_t partition);
File* get_root();
void open(File* file, std::string *filepath);
void close(File* file);
void* read(File *file);

}