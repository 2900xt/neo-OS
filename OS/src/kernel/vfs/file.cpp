
#include <drivers/fs/fat/fat.h>
#include <kernel/io/log.h>
#include <kernel/mem/paging.h>
#include <kernel/vfs/volume.h>

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
}
