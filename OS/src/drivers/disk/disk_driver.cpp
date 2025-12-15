#include "drivers/disk/ahci/ahci.h"

#include <drivers/fs/gpt.h>
#include <drivers/disk/disk_driver.h>
#include <kernel/io/log.h>

namespace disk
{

    rw_disk_t *disks[10];
    uint8_t disk_count;

    static const char *disk_driver_tag = "Disk Driver";

    void write(rw_disk_t *disk, uint32_t starting_lba, uint32_t sector_cnt, void *buffer)
    {
        int status;
        if (disk->type == diskTypes::AHCI)
        {
            AHCIDevice *driver = (AHCIDevice *)disk->driver;
            status = driver->write(starting_lba, sector_cnt, (uint8_t *)buffer);
        }
        else
        {
            log.e(
                disk_driver_tag,
                "Unknown Disk Type: %u", disk->type);
            return;
        }

        if (status != 0)
        {
            log.e(disk_driver_tag, "Disk Write Error: %d", status);
        }
    }

    void read(rw_disk_t *disk, uint64_t starting_lba, uint32_t sector_cnt, void *buffer)
    {
        int status;
        if (disk->type == diskTypes::AHCI)
        {
            AHCIDevice *driver = (AHCIDevice *)disk->driver;
            status = driver->read(starting_lba, sector_cnt, (uint8_t *)buffer);
        }
        else
        {
            log.e(disk_driver_tag, "Unknown Disk Type: %u", disk->type);
            return;
        }

        if (status != 0)
        {
            log.e(disk_driver_tag, "Disk Read Error: %d", status);
        }
    }

    filesystem::gpt_part_data *get_gpt(rw_disk_t *disk)
    {
        if (disk->type == diskTypes::AHCI)
        {
            AHCIDevice *driver = (AHCIDevice *)disk->driver;
            return driver->get_gpt();
        }
        else
        {
            log.e(disk_driver_tag, "Unable to find GPT data - Unknown Disk Type: %u", disk->type);
            return NULL;
        }
    }

    rw_disk_t *get_disk(uint8_t drive_num)
    {
        return disks[drive_num];
    }

}
