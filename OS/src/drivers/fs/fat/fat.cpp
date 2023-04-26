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

fat_dir_entry *FATPartition::search_dir(fat_dir_entry *first_entry, const char *filename)
{
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
            return nullptr;
        }

        if(std::strcmp(filename, current_file->dir_name, 11))                       //File found
        {
            return current_file;
        }
        current_file ++;

    }
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

fat_dir_entry *FATPartition::get_file(const char *filepath)
{
    int path_length = std::strlen(filepath);

    char *fmt_filepath = new char[path_length + 1];
    char *_ptr = fmt_filepath;
    
    read_root_dir();

    int dir_level = format_path(filepath, &fmt_filepath);
    fat_dir_entry *current_entry = root_dir;    //First entry in the root directory

    //Now find the current directory

    for(int i = 0; i < dir_level; i++)
    {
        char *dir_entry_name = new char[11];
        format_filename(fmt_filepath, dir_entry_name);

        //Find the next directory

        fat_dir_entry *old_entry = current_entry;

        current_entry = search_dir(current_entry, dir_entry_name);

        if(current_entry == nullptr)
        {
            std::klogf("Unable to find %s\n", dir_entry_name);
            return old_entry;
        }
        
        //Now read the next directory from current entry

        fat_dir_entry *new_entry = (fat_dir_entry*)read_file(current_entry);

        current_entry = new_entry;

        //Current entry holds the next first entry of the directory

        fmt_filepath += std::strlen(fmt_filepath) + 1;

        delete[] dir_entry_name;
    }

    //Now search the final directory for the target file

    char *file_name_fmt = new char[11];
    format_filename(fmt_filepath, file_name_fmt);

    fat_dir_entry *old_entry = current_entry;
    current_entry = search_dir(current_entry, file_name_fmt);

    if(current_entry == nullptr)    //The file doesn't exist
    {
        return old_entry;
    }

    delete[] file_name_fmt;
    delete[] _ptr;

    return current_entry;
}

void *FATPartition::open_file(const char *filepath)
{
    fat_dir_entry *file = get_file(filepath);

    if(!file) 
    {
        std::klogf("Error: unable to find file: %s\n", filepath);
        return nullptr;
    }

    void *buf = read_file(file);

    if(!buf)
    {
        std::klogf("Error: unable to read file: %s\n", filepath);
        return nullptr;
    }

    return buf;
}

fat_dir_entry *FATPartition::read_root_dir()
{
    if(root_dir)
    {
        kernel::free_pages(root_dir);
    }

    root_dir = (fat_dir_entry*)kernel::allocate_pages(1);
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

    return root_dir;

}

void FATPartition::create_file(const char *parent_dir_path, const char *filename, F32_ATTRIB attrib)
{

    char *filename_fmt = new char[11];

    format_filename(filename, filename_fmt);

    //Now go to the end of the parent directory

    fat_dir_entry *current_file = get_file(parent_dir_path);

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

        current_file ++;

    }

    current_file->file_size = 0;
    current_file->dir_attrib = (uint8_t)attrib;

    std::strcpy(current_file->dir_name, filename_fmt);

    delete[] filename_fmt;
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
    
}

FATPartition::~FATPartition()
{
}

}