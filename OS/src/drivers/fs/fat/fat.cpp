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

namespace FS 
{

static bios_param_block *read_bpb(DISK::rw_disk_t *device, int partition)
{
    uint32_t lba = DISK::get_gpt(device)->entries[partition].starting_lba;
    bios_param_block *data = (bios_param_block*)kernel::allocate_pages(1);
    mmap(data, data);
    DISK::read(device, lba, 1, data);
    return data;
}

#define MAX_FILENAME_LENGTH 11

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

void *FATPartition::read_file(fat_dir_entry *file)
{
    int page_count = file->file_size / 0x1000;
    void *_buffer = (void*)kernel::allocate_pages(page_count);
    uint8_t *buffer = (uint8_t*)_buffer;
    kernel::map_pages((uint64_t)buffer, (uint64_t)buffer, page_count);

    uint32_t current_cluster = file->first_cluster_h << 16 | file->first_cluster_l;
    do {
        if(current_cluster == 0x0FFFFFF7)
        {
            std::klogf("Bad cluster: %u\n", current_cluster);
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
    current_cluster *= 4;
    return *(uint32_t*)&fat[current_cluster] & ~0xF0000000;
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
            std::klogf("F32: FATAL Error: File not found!\n");
            return NULL;
        }

        current_entry = (fat_dir_entry*)read_file(current_entry);
        current_name += std::strlen(current_name) + 1;

        kernel::free_pages(current_dir);

        if(current_entry == NULL)
        {
            std::klogf("F32: FATAL Error: File not found!\n");
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

void *FATPartition::read_file(const char *filepath)
{
    fat_dir_entry *file = get_file(filepath);

    if(!file) 
    {
        std::klogf("File Does Not Exist! %s\n", filepath);
        return NULL;
    }

    void *buf = read_file(file);

    if(!buf)
    {
        std::klogf("File Read Error! %s\n", filepath);
        return NULL;
    }

    return buf;
}

void FATPartition::create_file(const char *parent_dir_path, const char *filename, uint8_t attrib)
{
    fat_dir_entry *parent_dir = get_file(parent_dir_path);
    parent_dir = (fat_dir_entry*)read_file(parent_dir);
    
    //Go to the end of the parent dir
    while(parent_dir->dir_name[0] != 0x00)
    {
        parent_dir++;
    }

    char *fmt_filename = new char[MAX_FILENAME_LENGTH];

    //Create a new entry
    parent_dir->dir_attrib = attrib;

    delete[] fmt_filename;
    
}

FATPartition::FATPartition(DISK::rw_disk_t *dev, int partition)
{
    this->dev = dev;
    this->parition = partition;
    this->bpb = read_bpb(dev, partition);
    if(bpb->magic_number != 0xAA55)
    {
        std::klogf("Invalid Magic Number: 0x%x on (hd%u, gpt%u)\n", bpb->magic_number, partition);
    }

    firstSector = DISK::get_gpt(dev)->entries[partition].starting_lba;
    
    sectorCount = bpb->total_sector_count_32;
    fatSize = bpb->fat_size_32;
    firstDataSector = bpb->reserved_sectors + (bpb->num_fats * fatSize);
    firstFatSector = bpb->reserved_sectors + firstSector;
    totalDataSectors = sectorCount - firstDataSector;
    totalClusters = totalDataSectors / bpb->sectors_per_cluster;

    //Read the FAT

    fat = (uint8_t*)kernel::allocate_pages(fatSize / 0x1000 + 1);
    kernel::map_pages((uint64_t)fat, (uint64_t)fat, fatSize / 0x1000 + 1);
    DISK::read(dev, firstFatSector, fatSize / bpb->bytes_per_sector + 1, fat);

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

}