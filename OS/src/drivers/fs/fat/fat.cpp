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
}__attribute__((packed));


static bios_param_block *read_bpb(DISK::rw_disk_t *device, int partition)
{
    uint32_t lba = DISK::get_gpt(device)->entries[partition].starting_lba;
    bios_param_block *data = (bios_param_block*)kernel::allocate_pages(1);
    mmap(data, data);
    DISK::read(device, lba, 1, data);
    return data;
}

void format_filename(const char *src, char *dest)
{

    memset_8(dest, MAX_FILENAME_LENGTH, ' ');

    for(int i = 0; i < MAX_FILENAME_LENGTH; i++)
    {
        if(*src == '\0')
        {
            break;
        }

        if(*src == '.')
        {
            i = 8;
            src++;
        }

        dest[i] = std::toUpper(*src);
        src++;
    }
}

fat_dir_entry *FATPartition::search_dir(fat_dir_entry *first_entry, const char *filename)
{
    fat_dir_entry *current_file = first_entry;

    //Format the filename
    char *fmt_filename = new char[MAX_FILENAME_LENGTH];
    format_filename(filename, fmt_filename);

    while (true) 
    {
    
        if((uint8_t)current_file->dir_name[0] == 0xE5 || current_file->dir_attrib == 0x0F)      //Unused entry  or Long file name
        {
            current_file++;
            continue;
        }

        if(current_file->dir_name[0] == 0x00)                                                   //Reaced end of directory
        {
            return NULL;
        }

        if(std::strcmp(fmt_filename, current_file->dir_name, 11))                       //File found
        {
            return current_file;
        }
        current_file ++;
    }

    delete [] fmt_filename;
    return NULL;
}

void *FATPartition::read_entry(fat_dir_entry *file)
{
    int page_count = file->file_size / 0x1000;
    void *_buffer = (void*)kernel::allocate_pages(page_count);
    uint8_t *buffer = (uint8_t*)_buffer;
    kernel::map_pages((uint64_t)buffer, (uint64_t)buffer, page_count);

    uint32_t current_cluster = file->first_cluster_h << 16 | file->first_cluster_l;
    do {
        if(current_cluster == 0x0FFFFFF7)
        {
            Log.e(fat_driver_tag, "Bad Cluster: %u", current_cluster);
            kernel::free_pages(_buffer);
            return NULL;
        }

        uint32_t lba = firstSector + firstDataSector + (current_cluster - 2) * bpb->sectors_per_cluster;
        DISK::read(dev, lba, bpb->sectors_per_cluster, buffer);

        buffer += bpb->bytes_per_sector * bpb->sectors_per_cluster;

        current_cluster = get_next_cluster(current_cluster);
    } while (current_cluster < 0x0FFFFFF8);

    return _buffer;
}

uint32_t FATPartition::get_next_cluster(int current_cluster)
{
    return fat[current_cluster] & ~0xF0000000;
}


int FATPartition::format_path(const char *_filepath, char **filepath)
{
    int pathLength = std::strlen(_filepath);

    *filepath = new char[pathLength + 1];

    int dir_level_count = 0;

    while(*_filepath == '/')
    {
        _filepath++;
    }

    for(int i = 0; i < pathLength; i++)
    {
        if(_filepath[i] == '/')
        {
            (*filepath)[i] = '\0';
            dir_level_count++;
            continue;
        }

        (*filepath)[i] = _filepath[i];
    }

    (*filepath)[pathLength] = -1;

    return dir_level_count;
}

fat_dir_entry *FATPartition::get_file(const char *filepath)
{
    int path_length = std::strlen(filepath);
    char *fmt_filepath = new char[path_length + 1];
    int dir_level = format_path(filepath, &fmt_filepath);
    fat_dir_entry *current_entry = root_dir;
    char *current_name = fmt_filepath;
    
    //Now find the current directory

    for(int i = 0; i < dir_level; i++)
    {
        fat_dir_entry *current_dir = current_entry;
        current_entry = search_dir(current_entry, current_name);

        if(current_entry == NULL)
        {
            Log.e(fat_driver_tag, "Unable to read parent directory");
            return NULL;
        }

        current_entry = (fat_dir_entry*)read_entry(current_entry);
        current_name += std::strlen(current_name) + 1;

        kernel::free_pages(current_dir);

        if(current_entry == NULL)
        {
            Log.e(fat_driver_tag, "Unable to find entry for parent directorys");
            return NULL;
        }
    }

    char *fmt_name = new char[MAX_FILENAME_LENGTH];
    format_filename(current_name, fmt_name);
    current_entry = search_dir(current_entry, fmt_name);

    delete[] fmt_name;
    delete[] fmt_filepath;
    return current_entry;
}

FATPartition::FATPartition(DISK::rw_disk_t *dev, int partition)
{
    this->dev = dev;
    this->parition = partition;
    this->bpb = read_bpb(dev, partition);
    if(bpb->magic_number != 0xAA55)
    {
        Log.e(
            fat_driver_tag,
            "Invalid Magic Number: 0x%x on (hd%u, gpt%u)", bpb->magic_number, partition);
    }

    firstSector = DISK::get_gpt(dev)->entries[partition].starting_lba;
    
    sectorCount = bpb->total_sector_count_32;
    fatSize = bpb->fat_size_32;
    firstDataSector = bpb->reserved_sectors + (bpb->num_fats * fatSize);
    firstFatSector = bpb->reserved_sectors + firstSector;
    totalDataSectors = sectorCount - firstDataSector;
    totalClusters = totalDataSectors / bpb->sectors_per_cluster;

    //Read the FAT

    fat = (uint32_t*)kernel::allocate_pages((fatSize * bpb->bytes_per_sector) / 0x1000 + 1);
    kernel::map_pages((uint64_t)fat, (uint64_t)fat, (fatSize * bpb->bytes_per_sector) / 0x1000 + 1);
    DISK::read(dev, firstFatSector, fatSize, fat);

    //Read the root directory
    root_dir = (fat_dir_entry*)kernel::allocate_pages(5);
    kernel::map_pages((uint64_t)root_dir, (uint64_t)root_dir, 5);
    fat_dir_entry *buf = root_dir;
    uint32_t current_cluster = firstDataSector;
    rootDirSize = 0;
    do {
        if(current_cluster == 0x0FFFFFF7)
        {
            Log.e(
                fat_driver_tag,
                "Bad cluster: %u", current_cluster);
            break;
        }

        uint32_t lba = firstSector + current_cluster * bpb->sectors_per_cluster;
        DISK::read(dev, lba, bpb->sectors_per_cluster, buf);

        buf += bpb->bytes_per_sector * bpb->sectors_per_cluster;
        current_cluster = get_next_cluster(current_cluster);
    } while (current_cluster < 0x0FFFFFF8);
}

FATPartition::~FATPartition()
{
    kernel::free_pages(fat);
    kernel::free_pages(bpb);
    kernel::free_pages(root_dir);
}

void FATPartition::convert_to_vfs(VFS::File *out, fat_dir_entry* in)
{
    out->file_size = in->file_size;
    out->last_write_time = in->last_write_time;
    out->last_write_date = in->last_access_date;
    out->time_created = in->time_created;
    out->date_created = in->date_created;
    out->last_access_date = in->last_access_date;

    out->fsinfo.drive_number = this->dev->disk_number;
    out->fsinfo.filesystem_id = VFS::FAT32;
    out->fsinfo.volume_number = this->parition;
    out->fsinfo.first_cluster = (in->first_cluster_h << 16) | (in->first_cluster_l);
}

void FATPartition::mount_dir(fat_dir_entry *first_entry, VFS::File *parent)
{
    /* Iterate through directory */
    VFS::File *current_child;
    fat_dir_entry *current_file = first_entry;
    while (true) 
    {
        if((uint8_t)current_file->dir_name[0] == 0xE5 || current_file->dir_attrib == 0x0F)      //Unused entry  or Long file name
        {
            current_file++;
            continue;
        }

        if(current_file->dir_name[0] == 0x00)                                                   //Reaced end of directory
        {
            break;
        }

        Log.v("VFS", "Mounting file: %s", current_file->dir_name);
        if(
            std::strcmp(current_file->dir_name, ". ", 2) || 
            std::strcmp(current_file->dir_name, ".. ", 3)
        )
        {
            current_file++;
            continue;
        }
        

        VFS::File *prev = current_child;
        current_child  = new VFS::File;
        convert_to_vfs(current_child, current_file);
        current_child->parent = parent;
        current_child->prev = prev;
        if(prev != NULL)
        {
            //sprev->next= current_child;
        }

        if (current_file->dir_attrib & F32_ATTRIB::DIRECTORY)
        {
            /*Recursively mount directories*/
            fat_dir_entry *first_entry = (fat_dir_entry*)this->read_entry(current_file);
            mount_dir(first_entry, current_child);
            kernel::free_pages(first_entry);
        }


        current_file++;
    }

    parent->child = current_child;
}

VFS::File *FATPartition::mount_fs()
{
    /* Create the root file */
    VFS::File *root = new VFS::File;

    root->fsinfo.drive_number = this->dev->disk_number;
    root->fsinfo.filesystem_id = VFS::FAT32;
    root->fsinfo.volume_number = this->parition;

    mount_dir(root_dir, root);
    return root;
}

}