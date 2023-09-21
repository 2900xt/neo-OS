#include "drivers/disk/disk_driver.h"
#include "drivers/fs/fat/fat.h"
#include "stdlib/assert.h"
#include <types.h>
#include <kernel/vfs/file.h>
#include <kernel/mem/mem.h>
#include <kernel/mem/paging.h>
#include <kernel/smp.h>
#include <stdlib/string.h>
#include <stdlib/stdio.h>

namespace VFS 
{

static const char *vfs_tag = "VFS";
static File* root;
static FS::FAT_partition *root_part;

void mount_root(DISK::rw_disk_t *disk, uint64_t partition)
{
    root_part = FS::mount_part(disk, partition, root);
}

void vfs_init()
{
    root = new File;
    mount_root(DISK::disks[0], 0);

    if(root_part == NULL)
    {
        Log.e(vfs_tag, "Error while mounting root!");
        kernel::panic();
    }
}

File* get_root()
{
    return root;
}

void open(File* file, std::string* filepath)
{
    int count;

    FS::fat_dir_entry *entry = FS::get_file_entry(root_part, filepath);

    file->fat_entry = entry;
    file->filename = std::string(*filepath->split('/', &count)[count - 1]);
    file->filesize = entry->file_size;
}

void* read(File* file)
{
    return read_cluster_chain(root_part, (FS::fat_dir_entry*)file->fat_entry);
}

void close(File* file)
{
    kfree(file->fat_entry);
}


}
