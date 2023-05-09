#pragma once
#include "drivers/fs/gpt.h"
#include <types.h>

namespace DISK
{

enum class diskTypes
{
    AHCI
};

struct rw_disk_t
{
    void *driver;
    diskTypes type;
    int disk_number;
};

extern rw_disk_t *disks[10];
extern uint8_t disk_count;

void write(rw_disk_t *disk, uint32_t starting_lba, uint32_t sector_cnt, void *buffer);
void read(rw_disk_t *disk, uint64_t starting_lba, uint32_t sector_cnt, void *buffer);
FS::gpt_part_data *get_gpt(rw_disk_t *disk);
}