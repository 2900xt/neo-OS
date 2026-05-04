#include <types.h>
#include "stdlib/structures/string.h"
#include <kernel/vfs/file.h>

namespace kernel
{
    struct disk_vol_t
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

    void vol_mgr_init();

    void register_vol(int drive, int part, disk_vol_t::FS_TYPE fs_type);
    disk_vol_t* get_vol_driver(int drive, int part);
    void get_file_entry(const stdlib::string& filepath, file_handle* handle);
};