#include "drivers/disk/ahci/ahci.h"
#include "stdlib/stdio.h"
#include <types.h>
#include <drivers/fs/gpt.h>
#include <drivers/disk/disk_driver.h>

namespace DISK
{
    
void write(rw_disk_t *disk, uint32_t starting_lba, uint32_t sector_cnt, void *buffer)
{
    int status;
    if(disk->type == diskTypes::AHCI)
    {
        AHCIDevice *driver = (AHCIDevice*)disk->driver;
        status = driver->write(starting_lba, sector_cnt, buffer);
    } else
    {
        std::klogf("Unknown Disk Type: %d\nFatal Read Error!\n\n", disk->type);
        return;
    }

    if(status != 0)
    {
        std::klogf("Disk Write Error: %d\n", status);
    }
}

void read(rw_disk_t *disk, uint64_t starting_lba, uint32_t sector_cnt, void *buffer)
{
    int status;
    if(disk->type == diskTypes::AHCI)
    {
        AHCIDevice *driver = (AHCIDevice*)disk->driver;
        status = driver->read(starting_lba, sector_cnt, buffer);
    } else
    {
        std::klogf("Unknown Disk Type: %d\nFatal Read Error!\n\n", disk->type);
        return;
    }

    if(status != 0)
    {
        std::klogf("Disk Read Error: %d\n", status);
    }
}

FS::gpt_part_data *get_gpt(rw_disk_t *disk)
{
    if(disk->type == diskTypes::AHCI)
    {
        AHCIDevice *driver = (AHCIDevice*)disk->driver;
        return driver->get_gpt();
    } else
    {
        std::klogf("Unknown Disk Type: %d\nUnable to get GPT\n\n", disk->type);
    }
}

}
