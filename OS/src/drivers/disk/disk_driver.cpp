#include "drivers/disk/ahci/ahci.h"
#include <drivers/fs/gpt.h>
#include <drivers/disk/disk_driver.h>
#include <kernel/io/log.h>

namespace disk
{
    rw_disk_t disks[10];

    static const char *disk_driver_tag = "Disk Driver";

    void write(rw_disk_t *disk, uint32_t starting_lba, uint32_t sector_cnt, void *buffer)
    {
        int status;
        if (disk->type == disk_type_t::AHCI)
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
        if (disk->type == disk_type_t::AHCI)
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
        if (disk->type == disk_type_t::AHCI)
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

    rw_disk_t *get_disk(uint8_t id)
    {
        return &disks[id];
    }

    rw_disk_t *register_disk(disk_type_t type, int disk_id, void *driver)
    {
        int first_null = -1;
        for(int i = 0; i < 10; i++)
        {
            if(disks[i].disk_number == disk_id && disks[i].type == type)
            {
               log.e(disk_driver_tag, "Unable to register disk 0x%x for driver 0x%x: Duplicate has already been registered.", disk_id, type);
               return NULL; 
            }

            if(disks[i].type == disk_type_t::EMPTY && first_null == -1)
            {
                first_null = i;
            }
        }

        if(first_null == -1)
        {
            log.e(disk_driver_tag, "Unable to register disk 0x%x for driver 0x%x: Too many disks mounted.", disk_id, type);
            return NULL;
        }

        disks[first_null].disk_number = disk_id;
        disks[first_null].type = type;
        disks[first_null].driver = driver;

        log.v(disk_driver_tag, "Successfully Registered disk 0x%x for driver 0x%x (num: 0x%x)", disk_id, type, first_null);
        return &disks[first_null];
    }
}