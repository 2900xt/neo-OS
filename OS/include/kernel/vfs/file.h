#pragma once
#include <stdlib/string.h>
#include <types.h>

#define FILE_READABLE       (1 << 0)
#define FILE_WRITABLE       (1 << 1)
#define FILE_EXECUTABLE     (1 << 2)

namespace VFS
{

enum file_types : uint8_t
{
    FILESYSTEM,
    DIRECTORY,
    DEVICE,
};

enum file_permissions : uint8_t
{
    READ    = 0x1,
    WRITE   = 0x2,
    EXECUTE = 0x4,
};

enum filesystem_type : uint8_t
{
    FAT32,
};

struct fsinfo_t 
{
    uint64_t drive;
    uint64_t partition;
    uint64_t starting_lba;

    filesystem_type fstype;
};

struct File
{
    std::string filename;
    uint8_t     permissions;
    uint8_t     file_type;

    File* youngest_child;
    File* parent;
    File* older_sibling;
    File* younger_sibling;

    void* opt_ptr;
    fsinfo_t fsinfo;
};


}