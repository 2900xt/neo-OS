#include "drivers/disk/disk_driver.h"
#include "drivers/fs/fat/fat.h"
#include "stdlib/assert.h"
#include <types.h>
#include <kernel/vfs/file.h>
#include <kernel/mem/mem.h>
#include <kernel/mem/paging.h>
#include <stdlib/string.h>
#include <stdlib/stdio.h>

namespace VFS 
{

static const char * vfs_tag = "VFS";
static File root;
void mount_root(DISK::rw_disk_t *disk, uint64_t partition, filesystem_id fs)
{
    if(fs != FAT32)
    {
        Log.e(vfs_tag, "Unable to mount non FAT32 media as root partition");
        return;
    }

    root.filename = "/";
    root.owner_name = "root";
    
    root.filetype = ROOT;
    root.permissions = READ;

    root.fsinfo.drive_number = disk->disk_number;
    root.fsinfo.volume_number = partition;
    root.fsinfo.filesystem_id = fs;
}

File *get_root()
{
    return &root;
}

File* open(const char *filepath)
{

}

const char* get_file_path(File *file)
{

}

void read(File *file, void *buffer)
{
    
}


}
