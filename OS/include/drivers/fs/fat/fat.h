#pragma once
#include "drivers/disk/ahci/ahci.h"
#include "drivers/disk/disk_driver.h"
#include <kernel/vfs/file.h>
#include <types.h>

namespace filesystem
{

    enum F32_ATTRIB : uint8_t
    {
        READ_ONLY = 0x1,  // Writes to this file should fail
        HIDDEN = 0x2,     // Normal directory listings shouldn't show this file
        SYSTEM = 0x4,     // This is owned by the operating system
        VOLUME_ID = 0x8,  // The name of this file is the label of the volume
        DIRECTORY = 0x10, // Indicates that this file is actually a container for other files
        ARCHIVE = 0x20,   // Used in backup-utillities
        LONG_NAME = READ_ONLY | HIDDEN | SYSTEM | VOLUME_ID,
    };

    struct bios_param_block
    {
        uint8_t rsv0[3]; // JMP SHORT 3C NOP
        char oemID[8];
        uint16_t bytes_per_sector;
        uint8_t sectors_per_cluster;
        uint16_t reserved_sectors;
        uint8_t num_fats;
        [[gnu::deprecated]]
        uint16_t root_dir_entry_count;
        [[gnu::deprecated]]
        uint16_t total_sector_count_16; // If set to 0, there are more than 65535 sectors and the actual value is in the large sector counts
        uint8_t media_type;
        [[gnu::deprecated]]
        uint16_t fat_size_16;
        uint16_t sector_per_track;
        uint16_t head_count;
        uint32_t hidden_sector_count;
        uint32_t total_sector_count_32;

        // Extended Boot Record (FAT32 Only)

        uint32_t fat_size_32;
        uint16_t flags;
        uint16_t FAT_version;
        uint32_t root_dir_cluster;
        uint16_t fs_info_sector;
        uint16_t backup_boot_sector;
        uint8_t rsv2[12];
        uint8_t drive_number;
        uint8_t rsv3;
        uint8_t signature;
        uint32_t volume_id;
        char volume_label[11];
        char fs_identifier[8];

        uint8_t boot_sector_code[420];
        uint16_t magic_number; // AA55h

    } __attribute__((packed));

#define FSINFO_SIGNATURE_1 0x41615252
#define FSINFO_SIGNATURE_2 0x61417272
#define FSINFO_SIGNATURE_3 0xAA550000

    struct fsinfo_fat32
    {
        uint32_t signature_1;
        uint8_t rsv0[480];
        uint32_t signature_2;
        uint32_t free_cluster_count;
        uint32_t cluster_search_start;
        uint8_t rsv1[12];
        uint32_t signature_3;
    } __attribute__((packed));

    struct fat_dir_entry
    {
        char dir_name[11];
        uint8_t dir_attrib;
        uint8_t rsv0; // Reserved by windows NT
        uint8_t time_created_ms;
        uint16_t time_created;
        uint16_t date_created;
        uint16_t last_access_date;
        uint16_t first_cluster_h;
        uint16_t last_write_time;
        uint16_t last_write_date;
        uint16_t first_cluster_l;
        uint32_t file_size;

    } __attribute__((packed));

    struct long_file_name
    {
        /*The order of this entry in the sequence of long file names*/
        uint8_t index;

        uint16_t data1[5];

        /* Always equal to 0x0F */
        uint8_t attrib;

        uint8_t entry_type;

        uint8_t checksum;

        uint16_t data2[6];

        uint16_t zero;

        uint16_t data3[2];
    } __attribute__((packed));

    struct FAT_partition
    {

        // Read this to get access to root dir
        fat_dir_entry root_dir;

        // Data Structures
        bios_param_block *bpb;
        fsinfo_fat32 *fsinfo;
        uint32_t *fat;

        uint32_t first_sector;
        uint32_t total_sectors;
        uint32_t fat_size;
        uint32_t first_data_sector;
        uint32_t first_fat_sector;
        uint32_t total_clusters;

        disk::rw_disk_t *dev;
        int partition;
    };

    FAT_partition *mount_part(disk::rw_disk_t *device, int partition, kernel::File *root);
    fat_dir_entry *get_file_entry(FAT_partition *partition, stdlib::string *filepath);
    void *read_cluster_chain(filesystem::FAT_partition *partition, fat_dir_entry *file_entry);
    void print_directory_contents(filesystem::FAT_partition *partition, fat_dir_entry *directory);
}