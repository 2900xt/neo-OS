#include <drivers/disk/disk_driver.h>
#include <drivers/fs/fat/fat.h>
#include <kernel/io/log.h>
#include <kernel/vfs/file.h>
#include <stdlib/structures/string.h>
#include <types.h>

#include "drivers/fs/gpt.h"
#include "kernel/mem/mem.h"
#include "kernel/mem/paging.h"

namespace filesystem {
static const char* fat_driver_tag = "FAT Driver";

// Reading and writing data structures

/*
This is the BIOS parameter block. It contains information about the device.
We should not write to it.
*/
bios_param_block* read_bpb(disk::rw_disk_t* device, int partition) {
    gpt_part_data* gpt = disk::get_gpt(device);
    uint32_t lba = gpt->entries[partition].starting_lba;
    bios_param_block* data = (bios_param_block*)kernel::allocate_pages(1);
    mmap(data, data);
    disk::read(device, lba, 1, data);
    return data;
}

void write_fsinfo(FAT_partition* partition) 
{
}

void read_fat(FAT_partition* partition) {
    uint64_t fat_page_count = (partition->fat_size * partition->bpb->bytes_per_sector) / 0x1000 + 1;
    uint32_t fat_lba = partition->first_fat_sector;
    partition->fat = (uint32_t*)kernel::allocate_pages(fat_page_count);
    kernel::map_pages((uint64_t)partition->fat, (uint64_t)partition->fat, fat_page_count);

    // Read the FAT
    disk::read(partition->dev, fat_lba, partition->fat_size, partition->fat);

    partition->fsinfo = (fsinfo_fat32*)kernel::allocate_pages(1);
    kernel::map_pages((uint64_t)partition->fsinfo, (uint64_t)partition->fsinfo, 1);
    uint32_t fsinfo_lba = partition->gpt->starting_lba + partition->bpb->fs_info_sector;

    // Read the fsinfo
    disk::read(partition->dev, fsinfo_lba, 1, partition->fsinfo);

    // Set flag to clean
    partition->fat_dirty = 0;
}

void write_fat(FAT_partition* partition) {
    uint32_t fat_lba = partition->first_fat_sector;
    uint32_t fsinfo_lba = partition->gpt->starting_lba + partition->bpb->fs_info_sector;

    // Write the FAT
    disk::write(partition->dev, fat_lba, partition->fat_size, partition->fat);

    // Write the FSINFO
    disk::read(partition->dev, fsinfo_lba, 1, partition->fsinfo);

    partition->fat_dirty = 0;
}

filesystem::FAT_partition* mount_part(disk::rw_disk_t* device, int part_num,
                                      kernel::file_handle* folder) {
    bios_param_block* bpb = read_bpb(device, part_num);

    if (bpb->magic_number != 0xAA55) {
        log.e(fat_driver_tag, "(hd%d, gpt%d): Invalid BPB Magic Number: 0x%x", device->disk_number,
              part_num, bpb->magic_number);
        return NULL;
    }

    filesystem::FAT_partition* partition = new filesystem::FAT_partition;
    partition->bpb = bpb;
    partition->gpt = &disk::get_gpt(device)->entries[part_num];   
    
    partition->dev = device;
    partition->partition = part_num;

    partition->first_sector =
        disk::get_gpt(partition->dev)->entries[partition->partition].starting_lba;   

    partition->total_sectors = bpb->total_sector_count_32;
    partition->fat_size = bpb->fat_size_32;
    partition->first_fat_sector = partition->first_sector + bpb->reserved_sectors;
    partition->first_data_sector =
        partition->first_fat_sector + (partition->fat_size * bpb->num_fats);
    partition->total_clusters = partition->total_sectors / bpb->sectors_per_cluster;  

    read_fat(partition);
    log.v(fat_driver_tag, "(hd%d, gpt%d): Available Clusters: 0x%x\tFirst Cluster: 0x%x",
          device->disk_number, part_num, partition->fsinfo->free_cluster_count,
          partition->fsinfo->cluster_search_start);


    if (partition->fsinfo->signature_3 != FSINFO_SIGNATURE_3 ||
        partition->fsinfo->signature_2 != FSINFO_SIGNATURE_2 ||
        partition->fsinfo->signature_1 != FSINFO_SIGNATURE_1) {
        log.e(fat_driver_tag, "(hd%d, gpt%d): Invalid FSINFO Magic Number: (0x%x, 0x%x, 0x%x)",
              device->disk_number, part_num, partition->fsinfo->signature_1,
              partition->fsinfo->signature_2, partition->fsinfo->signature_3);
        return NULL;
    }

    if (partition->fat == NULL) {
        log.e(fat_driver_tag, "(hd%d, gpt%d): Error trying to read FAT", device->disk_number,
              part_num);
    }

    // Get the root directory information

    uint32_t cluster = bpb->root_dir_cluster;
    do {
        partition->root_dir.file_size += bpb->sectors_per_cluster * bpb->bytes_per_sector;
        cluster = get_next_cluster(partition, cluster);
    } while (cluster < 0x0FFFFFF8);

    partition->root_dir.first_cluster_l = bpb->root_dir_cluster;
    partition->root_dir.dir_attrib = DIRECTORY | SYSTEM;
    partition->root_dir.dir_name[0] = '/';

    print_file_info(&partition->root_dir);
    print_directory_contents(partition, &partition->root_dir);

    folder->fat_entry = &partition->root_dir;
    folder->filename = stdlib::string("/");
    folder->filesize = partition->root_dir.file_size;

    return partition;
}
}  // namespace filesystem