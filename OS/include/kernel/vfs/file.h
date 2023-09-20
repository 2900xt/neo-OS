#pragma once
#include <stdlib/string.h>
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
    FAT32    = 0xF32,
};

struct fsinfo_t
{
    uint64_t drive_number;
    uint64_t volume_number;
    uint64_t filesystem_id;
    uint64_t first_cluster;
};

struct File 
{
    std::string filename;
    std::string owner_name;

    uint8_t filetype;
    uint8_t permissions;
    uint64_t file_size;
    
    uint16_t    last_write_time;
    uint16_t    last_write_date;
    uint16_t    time_created;
    uint16_t    date_created;
    uint16_t    last_access_date;

    /* Parent Dir */
    File *parent;

    /* Contemporary Dir */
    File *next;
    File *prev;

    /* Child directoriess */
    File *child;

    /* Disk information */
    fsinfo_t fsinfo;
};

void vfs_init();
void mount_root(DISK::rw_disk_t *disk, uint64_t partition, filesystem_id fs);
File *get_root();
File *open(const char *filepath);
void read(File *file, void *buffer);

}