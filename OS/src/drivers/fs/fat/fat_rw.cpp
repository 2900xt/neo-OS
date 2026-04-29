#include <drivers/disk/disk_driver.h>
#include <drivers/fs/fat/fat.h>
#include <kernel/io/log.h>
#include <kernel/vfs/file.h>
#include <stdlib/structures/string.h>
#include <stdlib/structures/list.h>
#include "kernel/mem/mem.h"
#include "kernel/mem/paging.h"
#include "stdlib/math.h"

namespace filesystem {

static const char* fat_driver_tag = "FAT Driver";

uint32_t get_next_cluster(filesystem::FAT_partition* partition, int current_cluster) 
{
    return partition->fat[current_cluster] & ~0xF0000000;
}

// Make sure to free the allocated pages after using them
int read_cluster_chain(filesystem::FAT_partition* partition, void* _buffer, uint32_t start_cluster) 
{
    uint32_t cluster = start_cluster;
    uint8_t* buffer = (uint8_t*)_buffer;

    while (cluster < 0x0FFFFFF8) {
        if (cluster == 0x0FFFFFF7) {
            log.e(fat_driver_tag, "(hd%d, gpt%d): Encountered bad sector while reading cluster 0x%x",
                  partition->dev->disk_number, partition->partition, cluster);
            return -1;
        }

        uint32_t lba =
            partition->first_data_sector + (cluster - 2) * partition->bpb->sectors_per_cluster;
        disk::read(partition->dev, lba, partition->bpb->sectors_per_cluster, buffer);

        buffer += partition->bpb->bytes_per_sector * partition->bpb->sectors_per_cluster;
        cluster = get_next_cluster(partition, cluster);

    }

    return 0;
}


void* read_file_entry(filesystem::FAT_partition* partition, fat_dir_entry* file_entry)
{
    int page_count = file_entry->file_size / 0x1000 + 1;
    void* buffer = kernel::allocate_pages(page_count);

    kernel::map_pages((uint64_t)buffer, (uint64_t)buffer, page_count);
    uint32_t cluster = ((uint32_t)file_entry->first_cluster_h << 16) | (file_entry->first_cluster_l);

    read_cluster_chain(partition, buffer, cluster);
    return buffer;
}

static int find_free_cluster(filesystem::FAT_partition* partition)
{
    //find a free cluster
    uint64_t cur = partition->fsinfo->cluster_search_start;

    if(cur == 0xFFFFFFFF)
    {
        cur = 2;
    }

    while(cur < partition->total_clusters)
    {
        if(partition->fat[cur] == 0x0) break;
        cur++;
    }

    partition->fsinfo->cluster_search_start = cur;
    return cur;
}

/*
Follows, writes, (and/or) appends a cluster chain of X bytes until the end 
*/

int write_cluster_chain(filesystem::FAT_partition* partition, void* _buffer, uint64_t size_bytes, uint64_t start_cluster)
{
    uint64_t cluster_size = partition->bpb->sectors_per_cluster * partition->bpb->bytes_per_sector;
    uint64_t cluster_count = (size_bytes + cluster_size - 1) / cluster_size;

    if(cluster_count > partition->fsinfo->free_cluster_count)
    {
        return -1;
    }

    partition->fsinfo->free_cluster_count -= cluster_count;

    //find a free cluster
    uint64_t first = start_cluster != -1 ? start_cluster : find_free_cluster(partition);
    uint32_t cur = first, prev = 0, next;
    int num = 0;

    uint8_t *buffer = (uint8_t*)_buffer;

    while(num < cluster_count)
    {
        if(prev)
        {
            partition->fat[prev] = cur;
        }

        uint32_t lba = partition->first_data_sector + (cur - 2) * partition->bpb->sectors_per_cluster;
        disk::write(partition->dev, lba, partition->bpb->sectors_per_cluster, buffer);
        buffer += partition->bpb->bytes_per_sector * partition->bpb->sectors_per_cluster;

        prev = cur;
        num++;

        next = partition->fat[cur];
        partition->fat[cur] = 0xFFFFFFFF;
        if(next == 0x00) 
        {
            cur = find_free_cluster(partition);
            if(cur == partition->total_clusters)
            {
                log.e(fat_driver_tag, "ERROR: FAT IS FULL");
                return -2;
            }
        }
        else 
        {
            cur = next;
        }
    }

    int freed = 0;
    uint32_t orphan = next;
    while (orphan != 0 && orphan < 0x0FFFFFF8)
    {
        if (orphan == 0x0FFFFFF7) break;
        next = get_next_cluster(partition, orphan);
        partition->fat[orphan] = 0;
        orphan = next;
        freed++;
    }

    partition->fsinfo->free_cluster_count += freed;

    write_fat(partition);

    log.v(fat_driver_tag, "Wrote 0x%x clusters starting from 0x%x", num, first);
    return first;
}


int write_file_entry(filesystem::FAT_partition* partition, fat_dir_entry* file_entry, void* buffer)
{
    uint32_t cluster = ((uint32_t)file_entry->first_cluster_h << 16) | (file_entry->first_cluster_l);
    return write_cluster_chain(partition, buffer, file_entry->file_size, cluster);
}


// searches for the requested entry in an ALREADY read directory
static fat_dir_entry* search_directory(fat_dir_entry* directory_data,
                                       const char* filename, uint32_t directory_size) {
    fat_dir_entry* current_entry = directory_data;

    while ((uint8_t*)current_entry < (uint8_t*)directory_data + directory_size) 
    {
        if (current_entry->dir_name[0] == 0x00) break;

        if ((uint8_t)current_entry->dir_name[0] == 0xE5 || current_entry->dir_attrib == LONG_NAME) {
            current_entry++;
            continue;
        }

        if (stdlib::strcmp(filename, current_entry->dir_name, 11)) {
            return current_entry;
        }

        current_entry++;
    }

    return NULL;
}

fat_dir_entry* get_f32_file_entry(FAT_partition* partition, stdlib::string* filepath) {
    assert(filepath->at(0) == '/');

    int dir_levels;
    stdlib::string** path = filepath->split('/', &dir_levels);

    fat_dir_entry* current_dir = &partition->root_dir;

    for (int i = 1; i < dir_levels; i++) {
        const char* fat_filename = filename_to_fat(*path[i]);

        //Read the next directory
        fat_dir_entry* cur_dir_data = (fat_dir_entry*)read_file_entry(partition, current_dir);

        uint32_t cluster = ((uint32_t)current_dir->first_cluster_h << 16) | (current_dir->first_cluster_l);
        uint32_t current_dir_size = get_file_size(partition, cluster);
        current_dir = search_directory(cur_dir_data, fat_filename, current_dir_size);

        if (current_dir == NULL) 
        {
            log.e(fat_driver_tag, "(hd%d, gpt%d): Directory not found: %s",
                  partition->dev->disk_number, partition->partition, filepath->c_str());
            return NULL;
        }

        current_dir = copy_dir_entry(current_dir);
        kernel::free_pages(cur_dir_data);
    }

    return current_dir;
}

uint32_t get_file_size(FAT_partition* partition, uint32_t starting_cluster) {
    uint32_t file_size = 0, cluster = starting_cluster;
    while (cluster < 0x0FFFFFF8) {
        file_size += partition->bpb->sectors_per_cluster * partition->bpb->bytes_per_sector;
        cluster = get_next_cluster(partition, cluster);
    } 

    return file_size;
}

// Updates the entry at `filepath` to `entry`
void update_file_entry(FAT_partition* partition, stdlib::string* filepath, fat_dir_entry* entry)
{
    assert(filepath->at(0) == '/');

    int dir_levels;
    stdlib::string** path = filepath->split('/', &dir_levels);

    stdlib::string file = *path[dir_levels-1];
    stdlib::string dir = stdlib::string("/");
    dir.resize(filepath->length());
    for(int i = 1; i < dir_levels-1; i++)
    {
        dir.push_back('/');
        dir.append(*path[i]);
    }

    //find the directory that leads into the file
    fat_dir_entry* parent_entry = get_f32_file_entry(partition, &dir);
    fat_dir_entry* parent_dir = (fat_dir_entry*)read_file_entry(partition, parent_entry);

    uint32_t cluster = ((uint32_t)parent_entry->first_cluster_h << 16) | (parent_entry->first_cluster_l);
    uint32_t current_dir_size = get_file_size(partition, cluster);

    fat_dir_entry* cur = search_directory(parent_dir, filename_to_fat(file),  current_dir_size);

    if(cur == NULL)
    {
        log.e(fat_driver_tag, "(hd%d, gpt%d): Directory not found when updating: %s",
            partition->dev->disk_number, partition->partition, filepath->c_str());
        return;
    }

    kernel::memcpy(cur, entry, sizeof(fat_dir_entry));

    write_file_entry(partition, parent_entry, parent_dir);
}

/* 
create_file:
    This function takes in a filepath
*/
int create_file(FAT_partition* partition, stdlib::string* filepath, fat_dir_entry* dir_entry, void* buffer = NULL)
{
    assert(filepath->at(0) == '/');

    int dir_levels;
    stdlib::string** path = filepath->split('/', &dir_levels);

    stdlib::string file = *path[dir_levels-1];
    stdlib::string dir = stdlib::string("/");
    dir.resize(filepath->length());
    for(int i = 1; i < dir_levels-1; i++)
    {
        dir.push_back('/');
        dir.append(*path[i]);
    }

    const char* fat_filename = filename_to_fat(file);
    assert(stdlib::strcmp(fat_filename, dir_entry->dir_name, 11));

    //find the directory that leads into the file
    fat_dir_entry* parent_entry = get_f32_file_entry(partition, &dir);
    fat_dir_entry* parent_dir = (fat_dir_entry*)read_file_entry(partition, parent_entry);
    fat_dir_entry* current_entry = parent_dir;

    //double-check for duplicate filenames
    uint32_t cluster = ((uint32_t)parent_entry->first_cluster_h << 16) | (parent_entry->first_cluster_l);
    uint32_t current_dir_size = get_file_size(partition, cluster);
    if(search_directory(parent_dir, fat_filename, current_dir_size))
    {
        log.e(fat_driver_tag, "Duplicate Filename: Unable to create file '%s'", fat_filename);
        return -1;
    }
    
    //in the directory, find an open slot
    bool found = false;
    while ((uint8_t*)current_entry < (uint8_t*)parent_dir + current_dir_size) 
    {
        if (current_entry->dir_name[0] == 0x00 || (uint8_t)current_entry->dir_name[0] == 0xE5) 
        {
            // Found an open slot
            found = true;
            break;
        }

        current_entry++;
    }

    //extend this directory entry
    if(!found)
    {
        parent_entry->file_size += 0x1000;
        update_file_entry(partition, &dir, parent_entry);
    }
    
    
    //write to the entry
    kernel::memcpy(current_entry, dir_entry, sizeof(fat_dir_entry));

    //if the buffer is NULL, initialize a blank cluster
    if(buffer == NULL)
    {
        current_entry->file_size = 0;
        current_entry->first_cluster_l = 0x0;
        current_entry->first_cluster_h = 0x0;
    }
    else 
    {
        int start_cluster = write_cluster_chain(partition, buffer, current_entry->file_size);
        if(start_cluster < 0) return -2;

        current_entry->first_cluster_l = start_cluster & 0xFFFF;
        current_entry->first_cluster_h = start_cluster >> 16;
    }

    // update the directory
    if(write_file_entry(partition, parent_entry, parent_dir) < 0) return -3;

    return 0;
}

void save_file(FAT_partition* partition, stdlib::string* filepath, fat_dir_entry* entry, void* buffer)
{
    update_file_entry(partition, filepath, entry);
    write_file_entry(partition, entry, buffer);
}

void delete_file(FAT_partition* partition, stdlib::string* filepath)
{
    assert(filepath->at(0) == '/');

    int dir_levels;
    stdlib::string** path = filepath->split('/', &dir_levels);

    stdlib::string file = *path[dir_levels-1];
    stdlib::string dir = stdlib::string("/");
    dir.resize(filepath->length());
    for(int i = 1; i < dir_levels-1; i++)
    {
        dir.push_back('/');
        dir.append(*path[i]);
    }

    const char* fat_filename = filename_to_fat(file);

    fat_dir_entry* parent_dir_entry = get_f32_file_entry(partition, &dir);
    fat_dir_entry* parent_dir = (fat_dir_entry*)read_file_entry(partition, parent_dir_entry);

    uint32_t parent_dir_cluster = ((uint32_t)parent_dir_entry->first_cluster_h << 16) | (parent_dir_entry->first_cluster_l);
    uint32_t current_dir_size = get_file_size(partition, parent_dir_cluster);
    fat_dir_entry* file_entry = search_directory(parent_dir, fat_filename, current_dir_size);

    if(file_entry == NULL)
    {
        log.e(fat_driver_tag, "FILE NOT FOUND (deleting): %s", filepath->c_str());
        return;
    }

    int freed = 0;
    file_entry->dir_name[0] = 0xE5;
    // update the directory
    write_file_entry(partition, parent_dir_entry, parent_dir);

    uint32_t start = ((uint32_t)file_entry->first_cluster_h << 16) | (file_entry->first_cluster_l);
    uint32_t cluster = start;
    while (cluster < 0x0FFFFFF8)
    {
        if (cluster == 0x0FFFFFF7) 
        {
            log.e(fat_driver_tag, "(hd%d, gpt%d): Encountered bad sector while deleting cluster 0x%x",
                  partition->dev->disk_number, partition->partition, cluster);
            return;
        }

        uint32_t next = get_next_cluster(partition, cluster);
        partition->fat[cluster] = 0;
        cluster = next;
        freed++;
    }

    partition->fsinfo->free_cluster_count += freed;
    partition->fsinfo->cluster_search_start = stdlib::min(start, partition->fsinfo->cluster_search_start);
    write_fat(partition);
}

}  // namespace filesystem