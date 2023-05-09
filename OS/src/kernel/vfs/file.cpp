#include "drivers/disk/disk_driver.h"
#include "drivers/fs/fat/fat.h"
#include "stdlib/assert.h"
#include <types.h>
#include <kernel/vfs/file.h>
#include <kernel/mem/mem.h>
#include <kernel/mem/paging.h>
#include <stdlib/string.h>
#include <stdlib/stdio.h>

#define trace_vfs(msg) std::klogf("VFS: %s\n", msg)

namespace VFS 
{

static File root;
static FS::FATPartition *root_part;

void mount_root(DISK::rw_disk_t *disk, uint64_t partition, filesystem_id fs)
{
    if(fs != FAT32)
    {
        trace_vfs("Unable to mount non-FAT32 media as root partition!");
        return;
    }

    root_part = new FS::FATPartition(disk, partition);
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

FS::FATPartition *get_root_part()
{
    return root_part;
}

File* open(const char *filepath)
{
    FS::fat_dir_entry *fat_file = get_root_part()->get_file(filepath);
    File *fp = new File;
    fp->file_size = fat_file->file_size;
    fp->filename = fat_file->dir_name;

    return fp;
}

const char* get_file_path(File *file)
{
    char* path = new char[100];

    File *current = file;
    while(current != NULL)
    {
        std::strcat(path, current->filename.c_str());
        current = current->parent;
    }

    return path;
}

void read(File *file, void *buffer)
{
    const char *fp = get_file_path(file);
    void *tmpbuf = get_root_part()->read_file(fp);
    
    if(!tmpbuf)
    {
        trace_vfs("File Not Found!");
        return;
    }

    memcpy(buffer, tmpbuf, file->file_size);
    kernel::free_pages(tmpbuf);
    delete[] fp;
}


}
