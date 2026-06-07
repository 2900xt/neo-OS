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

    int fopen(file_handle *file, const stdlib::string& filepath);
    void fclose(file_handle *file);
    size_t fread(void *buffer, size_t num_bytes, file_handle *file);
}