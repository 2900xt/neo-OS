
#include <drivers/fs/fat/fat.h>
#include <kernel/io/log.h>
#include <kernel/mem/paging.h>
#include <kernel/vfs/volume.h>

namespace kernel
{
    static const char *vfs_tag = "VFS";

    struct file_path
    {
        int disk, vol;
        stdlib::string path;
    };

    int open(file_handle *file, stdlib::string *filepath)
    {
        int count;

        void *entry;
        if (filepath->c_str()[0] == '/' && filepath->c_str()[1] == '\0') 
        {
            file->is_root = true;
        }
        else {
            file->is_root = false;
            entry = kernel::get_file_entry(disk, vol, filepath);
        }

        if (entry == NULL)
        {
            log.e(vfs_tag, "file not found: %s", filepath->c_str());
            return -1;
        }

        file->fat_entry = entry;
        file->filename = stdlib::string(*filepath->split('/', &count)[count - 1]);
        file->filesize = entry->file_size;

        file->is_dir = (entry->dir_attrib & filesystem::DIRECTORY) || file->is_root;

        return 0;
    }

    void *read(file_handle *file)
    {
        // Check if we've already read this file in
        if(file->data)
        {
            return file->data;
        }

        return kernel::read_file_entry((filesystem::fat_dir_entry *)file->fat_entry);
    }

    void close(file_handle *file)
    {
        // Free the data we've read
        if (file->data)
        {
            free_pages(file->data);
        }

        kfree(file->fat_entry);
    }
}
