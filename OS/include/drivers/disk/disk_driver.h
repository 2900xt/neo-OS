#pragma once
#include "drivers/fs/gpt.h"
#include <types.h>

namespace disk
{
    enum class disk_type_t
    {
        EMPTY   = 0,
        AHCI    = 1,
    };

    struct rw_disk_t
    {
        void *driver;
        disk_type_t type;
        int disk_number;
    };

    void write(rw_disk_t *disk, uint32_t starting_lba, uint32_t sector_cnt, void *buffer);
    void read(rw_disk_t *disk, uint64_t starting_lba, uint32_t sector_cnt, void *buffer);
    filesystem::gpt_part_data *get_gpt(rw_disk_t *disk);


    rw_disk_t *register_disk(disk_type_t type, int disk_id, void *driver);
    rw_disk_t *get_disk(uint8_t drive_num);
}