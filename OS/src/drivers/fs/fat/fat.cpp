#include "drivers/disk/ahci/ahci.h"
#include "drivers/fs/gpt.h"
#include "drivers/fs/gpt.h"
#include "kernel/mem/mem.h"
#include "kernel/mem/paging.h"
#include "stdlib/stdio.h"
#include <types.h>
#include <drivers/fs/fat/fat.h>

namespace FS 
{

static bios_param_block* read_bpb(DISK::AHCIDevice *device, int partition)
{
    uint32_t lba = device->get_gpt()->entries[partition].starting_lba;
    bios_param_block *data = (bios_param_block*)kernel::allocate_pages(1);
    mmap(data, data);
    device->read(lba, 1, data);
    return data;
}

int FATPartition::read_file(const char *filename, void **buffer)
{

    
    return 0;
}

FATPartition::FATPartition(DISK::AHCIDevice *dev, int parition)
{
    this->bpb = read_bpb(dev, parition);
    if(bpb->magic_number != 0xAA55)
    {
        std::klogf("Invalid Magic Number: 0x%x on (hd%d, gpt%d)\n", bpb->magic_number, dev->port_num, parition);
    }
    
    sectorCount = bpb->sector_count_f32;
    fatSize = bpb->sectors_per_fat;
    firstDataSector = bpb->reserved_sectors + (bpb->FAT_count * fatSize);
    firstFatSector = bpb->reserved_sectors;
    totalDataSectors = sectorCount - firstDataSector;
    //totalClusters = totalDataSectors / bpb->sectors_per_cluster;

    //Read the root directory
    fat_dir_entry *root_dir = (fat_dir_entry*)kernel::allocate_pages(1);
    mmap(root_dir, root_dir);
    dev->read(2, 1, root_dir);

    std::klogf("%s\n", root_dir->dir_name);
    
}



}