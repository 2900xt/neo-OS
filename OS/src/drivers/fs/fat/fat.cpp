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
#include <drivers/disk/disk_driver.h>
#include <kernel/vfs/file.h>

namespace FS 
{

static const char * fat_driver_tag = "FAT Driver";
static constexpr int MAX_FILENAME_LENGTH = 11;

static bios_param_block *read_bpb(DISK::rw_disk_t *device, int partition)
{
    uint32_t lba = DISK::get_gpt(device)->entries[partition].starting_lba;
    bios_param_block *data = (bios_param_block*)kernel::allocate_pages(1);
    mmap(data, data);
    DISK::read(device, lba, 1, data);
    return data;
}

static fsinfo_fat32* read_fsinfo(DISK::rw_disk_t *device, bios_param_block* bpb, int partition)
{
    uint32_t lba = DISK::get_gpt(device)->entries[partition].starting_lba + bpb->fs_info_sector;
    fsinfo_fat32 *data = (fsinfo_fat32*)kernel::allocate_pages(1);
    mmap(data, data);
    DISK::read(device, lba, 1, data);
    return data;
}

static void read_fat(FAT_partition* partition)
{
    uint64_t page_count = (partition->fat_size * partition->bpb->bytes_per_sector) / 0x1000 + 1;
    partition->fat = (uint32_t*)kernel::allocate_pages(page_count);
    kernel::map_pages((uint64_t)partition->fat, (uint64_t)partition->fat, page_count);
    //TODO: FINISH CODE
}

static void read_cluster_chain(FS::FAT_partition* partition, uint64_t starting_cluster)
{

}


FS::FAT_partition* mount_part(DISK::rw_disk_t *device, int part_num, VFS::File* folder)
{
    bios_param_block* bpb = read_bpb(device, part_num);

    if(bpb->magic_number != 0xAA55)
    {
        Log.e(fat_driver_tag, "Invalid BPB Magic Number: 0x%x", bpb->magic_number);
        return NULL;
    }

    fsinfo_fat32* fsinfo = read_fsinfo(device, bpb, part_num);

    if(
        fsinfo->signature_3 != FSINFO_SIGNATURE_3 ||
        fsinfo->signature_2 != FSINFO_SIGNATURE_2 ||
        fsinfo->signature_1 != FSINFO_SIGNATURE_1
    )
    {
        Log.e(fat_driver_tag, "Invalid FSINFO Magic Number: (0x%x, 0x%x, 0x%x)", fsinfo->signature_1, fsinfo->signature_2, fsinfo->signature_3);
        return NULL;
    }

    Log.v(fat_driver_tag, "Available Cluster Count: 0x%x\tFirst Free Cluster: 0x%x", fsinfo->free_cluster_count, fsinfo->cluster_search_start);

    FS::FAT_partition *partition = new FS::FAT_partition;
    partition->bpb = bpb;
    partition->fsinfo = fsinfo;

    partition->dev = device;
    partition->partition = part_num;

    partition->total_sectors = bpb->total_sector_count_32;
    partition->fat_size = bpb->fat_size_32;
    partition->first_data_sector = bpb->reserved_sectors + (partition->fat_size * bpb->num_fats);
    partition->first_fat_sector = bpb->reserved_sectors;
    partition->data_sectors = partition->total_sectors - partition->first_data_sector;
    partition->total_clusters = partition->total_sectors / bpb->sectors_per_cluster;

    return partition;
}


}