#include <kernel/vfs/volume.h>
#include <kernel/io/log.h>
#include "drivers/disk/disk_driver.h"
#include "drivers/fs/fat/fat.h"
#include "kernel/mem/mem.h"
#include "kernel/vfs/file.h"
#include "stdlib/structures/string.h"

#define MAX_VOL 10

namespace kernel 
{
    static disk_vol_t volumes[MAX_VOL];
    disk_vol_t* get_vol_driver(int drive, int part)
    {
        for(int i = 0; i < MAX_VOL; i++)
        {
            if(volumes[i].drive_number == drive && volumes[i].gpt_partition_num == part)
            {
                return &volumes[i];
            }
        }

        log.e("VOLUME MGR", "Unable to find volume (0x%x, 0x%x)", drive, part);
        return NULL;
    }

    // Creates a filesystem driver for `fs_type` at drive
    void register_vol(int drive, int part, disk_vol_t::FS_TYPE fs_type)
    {
        int idx = -1;
        for(int i = 0; i < MAX_VOL; i++)
        {
            if(volumes[i].driver == NULL) 
            {
                idx = i;
                break;
            }
        }

        if(idx == -1)
        {
            log.e("VOLUME MGR", "Too many volumes mounted.");
            return;
        }

        volumes[idx].drive_number = drive;
        volumes[idx].gpt_partition_num = part;
        volumes[idx].filesys_type = fs_type;
        volumes[idx].vol_index = idx;
        switch (fs_type)
        {
        case disk_vol_t::FAT32:
            volumes[idx].driver = filesystem::mount_part(disk::get_disk(drive), part);
            break;
        case disk_vol_t::RAW:
            log.v("VOLUME MGR", "Warning: trying to mount a RAW volume!");    
            break;
        }
    }

    void vol_mgr_init()
    {
        kernel::memset(volumes, sizeof(disk_vol_t) * MAX_VOL, 0);
    }

    void get_file_entry(const stdlib::string& filepath, file_handle* handle)
    {
        int drive = filepath[0] - '0';
        int part = filepath[2] - '0';
        stdlib::string vol_path = filepath.substr(3);
        disk_vol_t* driver = get_vol_driver(drive, part);
        switch(driver->filesys_type)
        {
            case disk_vol_t::FAT32:
            {
                filesystem::fat_dir_entry* file_entry = filesystem::get_f32_file_entry((filesystem::FAT_partition*)driver->driver, vol_path);
                handle->file_entry = file_entry;
                handle->data = NULL;
                handle->filepath = filepath;
                handle->attrib = file_entry->dir_attrib;
                handle->filesize = file_entry->file_size;
                return;
            }
            default:
                log.e("VOLUME MGR", "trying to read a RAW volume!");    
                return;
        }
    }
};