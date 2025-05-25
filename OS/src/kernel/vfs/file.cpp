#include <kernel/kernel.h>
#include <stdlib/stdlib.h>
#include <drivers/fs/fat/fat.h>

namespace kernel
{
    static const char *vfs_tag = "VFS";
    static File *root;
    static filesystem::FAT_partition *root_part;

    void mount_root(disk::rw_disk_t *disk, uint64_t partition)
    {
        root_part = filesystem::mount_part(disk, partition, root);
    }

    void vfs_init()
    {
        root = new File;
        mount_root(disk::disks[0], 0);

        if (root_part == NULL)
        {
            log::e(vfs_tag, "Error while mounting root!");
            kernel::panic();
        }
    }

    File *get_root()
    {
        return root;
    }

    void open(File *file, stdlib::string *filepath)
    {
        int count;

        filesystem::fat_dir_entry *entry = filesystem::get_file_entry(root_part, filepath);

        file->fat_entry = entry;
        file->filename = stdlib::string(*filepath->split('/', &count)[count - 1]);
        file->filesize = entry->file_size;
    }

    void *read(File *file)
    {
        return read_cluster_chain(root_part, (filesystem::fat_dir_entry *)file->fat_entry);
    }

    void close(File *file)
    {
        kfree(file->fat_entry);
    }

}
