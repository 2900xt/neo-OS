
#pragma once
#include "drivers/disk/ahci/ahci.h"
#include <types.h>

namespace FS
{

enum class F32_ATTRIB : uint8_t
{
    READ_ONLY   = 0x1,  //Writes to this file should fail
    HIDDEN      = 0x2,  //Normal directory listings shouldn't show this file
    SYSTEM      = 0x4,  //This is owned by the operating system
    VOLUME_ID   = 0x8,  //The name of this file is the label of the volume
    DIRECTORY   = 0x10, //Indicates that this file is actually a container for other files
    ARCHIVE     = 0x20, //Used in backup-utillities
    LONG_NAME   = READ_ONLY | HIDDEN | SYSTEM | VOLUME_ID,
};

struct bios_param_block
{
    uint8_t         rsv0[3];    //JMP SHORT 3C NOP
    char            oemID[8];
    uint16_t        bytes_per_sector;
    uint8_t         sectors_per_cluster;
    uint16_t        reserved_sectors;
    uint8_t         FAT_count;
    [[gnu::deprecated]] uint16_t        root_dir_entry_count;    
    [[gnu::deprecated]] uint16_t        total_sector_count;     //If set to 0, there are more than 65535 sectors and the actual value is in the large sector counts
    uint8_t         media_type;
    [[gnu::deprecated]] uint16_t        fat_size;
    uint16_t        sector_per_track;
    uint16_t        head_count;
    uint32_t        hidden_sector_count;
    uint32_t        sector_count_f32;

    //Extended Boot Record (FAT32 Only)

    uint32_t        sectors_per_fat;
    uint16_t        flags;
    uint16_t        FAT_version;
    uint32_t        root_dir_cluster;
    uint16_t        fs_info_sector;
    uint16_t        backup_boot_sector;
    uint8_t         rsv2[12];
    uint8_t         drive_number;
    uint8_t         rsv3;
    uint8_t         signature;
    uint32_t        volume_id;
    char            volume_label[11];
    char            fs_identifier[8];

    uint8_t         boot_sector_code[420];
    uint16_t        magic_number;           //AA55h

}__attribute__((packed));

struct fat_dir_entry
{
    char        dir_name[11];
    F32_ATTRIB  dir_attrib;
    uint8_t     rsv0;           //Reserved by windows NT
    uint8_t     time_created_ms;
    uint16_t    first_cluster_h;
    uint16_t    last_write_time;
    uint16_t    last_write_date;
    uint16_t    first_cluster_l;
    uint32_t    file_size;

}__attribute__((packed));


void* read_file(DISK::AHCIDevice *device, int partition);

class FATPartition
{

fat_dir_entry *root_dir;
bios_param_block *bpb;

uint32_t sectorCount;
uint32_t fatSize;
uint32_t firstDataSector;
uint32_t firstFatSector;
uint32_t totalDataSectors;
uint32_t totalClusters;

public:
    FATPartition(DISK::AHCIDevice *dev, int parition);

    int read_file(const char *filename, void **buffer);
};

}