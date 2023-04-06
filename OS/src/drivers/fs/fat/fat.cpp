#include "drivers/disk/ahci/ahci.h"
#include "drivers/fs/gpt.h"
#include "drivers/fs/gpt.h"
#include "kernel/mem/mem.h"
#include "kernel/mem/paging.h"
#include "limine/limine.h"
#include "stdlib/stdio.h"
#include "stdlib/string.h"
#include <types.h>
#include <drivers/fs/fat/fat.h>

namespace FS 
{

static bios_param_block *read_bpb(DISK::AHCIDevice *device, int partition)
{
    uint32_t lba = device->get_gpt()->entries[partition].starting_lba;
    bios_param_block *data = (bios_param_block*)kernel::allocate_pages(1);
    mmap(data, data);
    device->read(lba, 1, data);
    return data;
}

const char *format_filename()
{
    return nullptr;
}

int FATPartition::read_file(const char *filename, void **buffer)
{
    //Search the root directory
    fat_dir_entry *current_file = root_dir;
    bool file_found = false;
    for(int entry = 0; entry < rootDirSize; entry++)
    {
        file_found = std::strcmp(filename, current_file->dir_name, 11);
        if(file_found)
        {
            break;
        }

        std::klogf("%d\n", -267);

        current_file += 0x2;
    }

    if(!file_found)
    {
        std::klogf("File not found: %s\n", filename);
    }
    
    return 0;
}

uint32_t FATPartition::get_next_cluster(int current_cluster)
{
    current_cluster *= 4;
    return *(uint32_t*)&fat[current_cluster] & ~0xF0000000;
}

FATPartition::FATPartition(DISK::AHCIDevice *dev, int partition)
{
    this->bpb = read_bpb(dev, partition);
    if(bpb->magic_number != 0xAA55)
    {
        std::klogf("Invalid Magic Number: 0x%x on (hd%u, gpt%u)\n", bpb->magic_number, dev->port_num, partition);
    }

    firstSector = dev->get_gpt()->entries[partition].starting_lba;
    
    sectorCount = bpb->total_sector_count_32;
    fatSize = bpb->fat_size_32;
    firstDataSector = bpb->reserved_sectors + (bpb->num_fats * fatSize);
    firstFatSector = bpb->reserved_sectors + firstSector;
    totalDataSectors = sectorCount - firstDataSector;
    totalClusters = totalDataSectors / bpb->sectors_per_cluster;

    //Read the FAT

    fat = (uint8_t*)kernel::allocate_pages(fatSize / 0x1000 + 1);
    kernel::map_pages((uint64_t)fat, (uint64_t)fat, fatSize / 0x1000 + 1);
    dev->read(firstFatSector, fatSize / bpb->bytes_per_sector + 1, fat);

    //Read the root directory
    
    root_dir = (fat_dir_entry*)kernel::allocate_pages(5);
    mmap(root_dir, root_dir);
    fat_dir_entry *buf = root_dir;
    uint32_t current_cluster = firstDataSector;
    rootDirSize = 0;
    do {
        if(current_cluster == 0x0FFFFFF7)
        {
            std::klogf("Bad cluster: %u\n", current_cluster);
            break;
        }

        uint32_t lba = firstSector + current_cluster * bpb->sectors_per_cluster;
        int sts = dev->read(lba, bpb->sectors_per_cluster, buf);

        buf += bpb->bytes_per_sector * bpb->sectors_per_cluster;
        current_cluster = get_next_cluster(current_cluster);
        rootDirSize += (bpb->bytes_per_sector * bpb->sectors_per_cluster) / sizeof(fat_dir_entry);
    } while (current_cluster < 0x0FFFFFF8);
}

FATPartition::~FATPartition()
{
    kernel::free_pages(fat);
    kernel::free_pages(bpb);
    kernel::free_pages(root_dir);
}



}