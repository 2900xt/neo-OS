#include <types.h>

namespace kernel
{
    class disk_vol_t
    {
        enum FS_TYPE
        {
            RAW,
            FAT32,
        } filesys_type;

        // -1 if not mounted
        int vol_index;
        int drive_number;
        int gpt_partition_num;

        void* driver;
    };

    int register_vol(disk_vol_t* new_vol);
    disk_vol_t* get_vol_by_idx(int idx);
};