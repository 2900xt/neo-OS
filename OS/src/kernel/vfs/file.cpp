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
static File *root;
static 

void mount_root(DISK::rw_disk_t *disk, uint64_t partition, filesystem_id fs)
{

}

File *get_root()
{
    return root;
}

File* open(const char *filepath)
{

}

void read(File *file, void *buffer)
{
    
}

const char* get_file_path(File *file)
{

}

}
