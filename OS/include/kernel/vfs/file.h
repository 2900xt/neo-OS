#pragma once
#include <stdlib/string.h>
#include "drivers/fs/fat/fat.h"
#include "drivers/disk/disk_driver.h"
#include <types.h>

#define FILE_READABLE       (1 << 0)
#define FILE_WRITABLE       (1 << 1)
#define FILE_EXECUTABLE     (1 << 2)

namespace VFS
{

enum file_types : uint8_t
{
    PHYS_FILE,
    VIRT_FILE,
    PHYS_DIR,
    VIRT_DIR,
    DEVICE,
    RESERVED,
    ROOT,
};

enum file_permissions : uint8_t
{
    READ     = (1 << 0),
    WRITE    = (1 << 1),
    EXECUTE  = (1 << 2),
};

enum filesystem_id : uint64_t 
{
    FAT32    = 0xFA5432,
};

struct fsinfo_t
{
    uint64_t drive_number;
    uint64_t volume_number;
    uint64_t filesystem_id;
};

struct File 
{
    std::string filename;
    std::string owner_name;

    uint8_t filetype;
    uint8_t permissions;
    uint64_t file_size;

// Parent Dir
    File *parent;

// Contemporary Dir
    File *next;
    File *prev;

// Child directoriess
    File *child;

    fsinfo_t fsinfo; 
};

void mount_root(DISK::rw_disk_t *disk, uint64_t partition, filesystem_id fs);
FS::FATPartition *get_root_part();
File *get_root();
File* open(const char *filepath);
void read(File *file, void *buffer);

}