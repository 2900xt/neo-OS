
#include "kernel/vfs/file.h"
#include <drivers/fs/fat/fat.h>
#include <kernel/io/log.h>
#include <kernel/mem/paging.h>
#include <kernel/vfs/volume.h>
#include "stdlib/math.h"

namespace kernel
{
    static const char *vfs_tag = "VFS";

    int fopen(file_handle *file, const stdlib::string& filepath)
    {
        get_file_entry(filepath, file);

        if (file->file_entry == NULL)
        {
            log.e(vfs_tag, "file not found: %s", filepath.c_str());
            return -1;
        }

        return 0;
    }

    void fclose(file_handle *file)
    {
        // Free the data we've read
        if (file->data)
        {
            free_pages(file->data);
        }

        kfree(file->file_entry);
    }

    size_t fread(void *buffer, size_t num_bytes, file_handle *file)
    {
        if (!file->data)
        {
            read_file_data(file);
        }

        size_t bytes_read = stdlib::min(num_bytes, file->filesize - file->offset);
        memcpy(buffer, (uint8_t*)file->data + file->offset, bytes_read);
        return bytes_read;
    }

    size_t fwrite(const void *buffer, size_t num_bytes, file_handle *file)
    {
        return num_bytes;
    }

#define SEEK_SET -1
#define SEEK_CUR 0
#define SEEK_END 1

    int fseek(file_handle *file, long offset, int whence = SEEK_SET)
    {
        switch(whence)
        {
            case SEEK_SET:
            {
                if(offset > file->filesize) return 1;
                file->offset = offset;
                return 0;
            }
            case SEEK_CUR:
            {
                if(offset + file->offset > file->filesize) return 1;
                file->offset = offset + file->offset;
                return 0;
            }
            case SEEK_END:
            {
                if(offset > file->filesize) return 1;
                file->offset = file->filesize - offset;
                return 0;
            }
        }

        return -67;
    }
}
