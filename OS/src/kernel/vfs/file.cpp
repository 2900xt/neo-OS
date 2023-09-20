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

static const char * vfs_tag = "VFS";
static File *root;
static FS::FAT_partition *root_part;

void mount_root(DISK::rw_disk_t *disk, uint64_t partition, filesystem_id fs)
{
    switch(fs)
    {
        case FAT32:
            root_part = FS::mount_part(disk, partition, root);
    }
}

void vfs_init()
{
    root = new File;
    mount_root(DISK::disks[0], 0, FAT32);

    if(root_part == NULL)
    {
        Log.e(vfs_tag, "Error while mounting root!");
        kernel::panic();
    }
}

File *get_root()
{
    return root;
}

File* open(const char *filepath)
{
    return NULL;
}

void read(File *file, void *buffer)
{
    
}

const char* get_file_path(File *file)
{
    return NULL;
}

}
