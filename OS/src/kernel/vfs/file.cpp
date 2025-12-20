#include "kernel/proc/smp.h"
#include <drivers/fs/fat/fat.h>
#include <kernel/io/log.h>

namespace kernel
{
    static const char *vfs_tag = "VFS";
    static file_handle *root;
    filesystem::FAT_partition *root_part;

    void mount_root(disk::rw_disk_t *disk, uint64_t partition)
    {
        root_part = filesystem::mount_part(disk, partition, root);
    }

    void vfs_init()
    {
        root = new file_handle;
        mount_root(disk::disks[0], 0);

        if (root_part == NULL)
        {
            panic(stdlib::string("Failed to mount root filesystem!"));
        }
    }

    file_handle *get_root()
    {
        return root;
    }

    int open(file_handle *file, stdlib::string *filepath)
    {
        int count;

        filesystem::fat_dir_entry *entry;
        if (filepath->c_str()[0] == '/' && filepath->c_str()[1] == '\0') {
            file->is_root = true;
            entry = (filesystem::fat_dir_entry *)root->fat_entry;
        }
        else {
            file->is_root = false;
            entry = filesystem::get_file_entry(root_part, filepath);
        }

        if (entry == NULL)
        {
            log.e(vfs_tag, "file_handle not found: %s", filepath->c_str());
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
        return read_cluster_chain(root_part, (filesystem::fat_dir_entry *)file->fat_entry);
    }

    void close(file_handle *file)
    {
        kfree(file->fat_entry);
    }

}
