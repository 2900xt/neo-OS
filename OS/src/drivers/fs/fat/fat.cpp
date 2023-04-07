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

void format_filename(const char *src, char *dest)
{
    int i;
    bool noExt = false;
    for(i = 0; i < 8; i++)
    {
        if(src[i] == '.') break;
        if(src[i] == '\0') 
        {
            noExt = true;
            break;
        }

        char caps = src[i];

        if(caps >= 97) caps -= 32;

        dest[i] = caps;
    }

    int file_ext_start = i + 1;

    for(; i < 8; i++)
    {
        dest[i] = ' ';
    }

    //File extension
    for(int j = 0; j < 3; j++)
    {
        if(noExt) break;
        if(src[file_ext_start] == '\0') break;
        char caps = src[file_ext_start];
        if(caps >= 97) caps -= 32;
        dest[i++] = caps;

        file_ext_start++;
    }


    for(; i < 11; i++)
    {
        dest[i] = ' ';
    }
}

fat_dir_entry *FATPartition::search_dir(fat_dir_entry *directory, const char *filename)
{
    fat_dir_entry *current_file = directory;
    bool file_found = false;

    while (true) 
    {
    
        if((uint8_t)current_file->dir_name[0] == 0xE5)      //Unused entry
        {
            current_file++;
            continue;
        }

        if(current_file->dir_name[0] == 0x00)               //Reaced end of directory
        {
            break;
        }

        if(current_file->dir_attrib == 0x0F)                //Long file name entry
        {
            current_file++;
            continue;
        }

        file_found = std::strcmp(filename, current_file->dir_name, 11);
        
        if(file_found)
        {
            break;
        }

        current_file ++;
    }

    if(!file_found)
    {
        std::klogf("File not found: %s\n",  filename);
        return nullptr;
    }

    return current_file;
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
            return nullptr;
        }

        uint32_t lba = firstSector + firstDataSector + (current_cluster - 2) * bpb->sectors_per_cluster;
        int sts = dev->read(lba, bpb->sectors_per_cluster, buffer);

        if(sts != 0)
        {
            std::klogf("Error reading sector %u on (hd%u, gpt%u)n", lba, dev->port_num, parition);
            kernel::free_pages(_buffer);
            return nullptr;
        }

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

FATPartition::FATPartition(DISK::AHCIDevice *dev, int partition)
{
    this->dev = dev;
    this->parition = partition;
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
    } while (current_cluster < 0x0FFFFFF8);
}

FATPartition::~FATPartition()
{
    kernel::free_pages(fat);
    kernel::free_pages(bpb);
    kernel::free_pages(root_dir);
}

int FATPartition::format_path(const char *_filepath, char **filepath)
{
    int pathLength = std::strlen(_filepath);

    *filepath = new char[pathLength + 1];

    int dir_level_count = 0;

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

void *FATPartition::open_file(const char *_filepath)
{
    //Split up the filepath into filenames

    char *filepath;
    int dir_level_count = format_path(_filepath, &filepath);
    char *const filepath_org = filepath;

    //Now, normalize the paths one by one and read directories

    fat_dir_entry *directory = root_dir;

    for(int i = 0; i < dir_level_count; i++)
    {
        char *normalized_dir = new char[11];

        int current_dir_len = std::strlen(filepath);

        format_filename(filepath, normalized_dir);

        fat_dir_entry *entry = search_dir(directory, normalized_dir);

        if(directory != root_dir) kernel::free_pages(directory);

        directory = (fat_dir_entry*)read_file(entry);

        filepath += current_dir_len + 1;

        delete[] normalized_dir;
    }

    //Finally, read the file from the directory

    char *normalized_file = new char[11];
    format_filename(filepath, normalized_file);

    fat_dir_entry *file = search_dir(directory, normalized_file);

    if(!file) 
    {
        std::klogf("Error: unable to find file\n");
        return nullptr;
    }

    void *buf = read_file(file);

    if(!buf)
    {
        std::klogf("Error: unable to read file\n");
        return nullptr;
    }

    delete[] normalized_file;
    delete[] filepath_org;

    return buf;
}

void FATPartition::create_file(const char *_filepath)
{
    //Split up the filepath into filenames

    char *filepath;
    int dir_level_count = format_path(_filepath, &filepath);
}

}