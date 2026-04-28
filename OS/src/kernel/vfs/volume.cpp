#include <kernel/vfs/volume.h>
#include <kernel/io/log.h>
#include "kernel/mem/mem.h"

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

    void register_vol(int drive, int part, disk_vol_t::FS_TYPE fs_type)
    {

    }

    void vol_mgr_init()
    {
        kernel::memset(volumes, sizeof(disk_vol_t) * MAX_VOL, 0);
    }
};