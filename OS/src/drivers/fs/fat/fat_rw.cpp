#include <drivers/disk/disk_driver.h>
#include <drivers/fs/fat/fat.h>
#include <kernel/io/log.h>
#include <kernel/vfs/file.h>
#include <stdlib/structures/string.h>
#include "kernel/mem/paging.h"

namespace filesystem {

static const char* fat_driver_tag = "FAT Driver";

uint32_t get_next_cluster(filesystem::FAT_partition* partition, int current_cluster) {
    return partition->fat[current_cluster] & ~0xF0000000;
}

// Make sure to free the allocated pages after using them
void* read_cluster_chain(filesystem::FAT_partition* partition, fat_dir_entry* file_entry) {
    int page_count = file_entry->file_size / 0x1000 + 1;
    void* buffer = kernel::allocate_pages(page_count);
    uint8_t* ptr = (uint8_t*)buffer;

    kernel::map_pages((uint64_t)buffer, (uint64_t)buffer, page_count);

    uint32_t cluster =
        ((uint32_t)file_entry->first_cluster_h << 16) | (file_entry->first_cluster_l);
    do {
        if (cluster == 0x0FFFFFF7) {
            log.e(fat_driver_tag, "(hd%d, gpt%d): Encountered bad sector while reading %s",
                  partition->dev->disk_number, partition->partition, file_entry->dir_name);
            kernel::free_pages(buffer);
            return NULL;
        }

        uint32_t lba =
            partition->first_data_sector + (cluster - 2) * partition->bpb->sectors_per_cluster;
        disk::read(partition->dev, lba, partition->bpb->sectors_per_cluster, ptr);

        ptr += partition->bpb->bytes_per_sector * partition->bpb->sectors_per_cluster;
        cluster = get_next_cluster(partition, cluster);

    } while (cluster < 0x0FFFFFF8);

    return buffer;
}

// Don't forget to kfree the dir entry
// Reads a directory into memory and searches for the requested entry
static fat_dir_entry* search_directory(FAT_partition* partition, fat_dir_entry* directory,
                                       const char* filename) {
    void* buffer = read_cluster_chain(partition, directory);
    fat_dir_entry* current_entry = (fat_dir_entry*)buffer;

    while (true) {
        if (current_entry->dir_name[0] == 0x00) break;

        if ((uint8_t)current_entry->dir_name[0] == 0xE5 || current_entry->dir_attrib == LONG_NAME) {
            current_entry++;
            continue;
        }

        if (stdlib::strcmp(filename, current_entry->dir_name, 11)) {
            fat_dir_entry* copy_of_entry = copy_dir_entry(current_entry);
            kernel::free_pages(buffer);
            return copy_of_entry;
        }

        current_entry++;
    }

    kernel::free_pages(buffer);
    return NULL;
}

fat_dir_entry* get_file_entry(FAT_partition* partition, stdlib::string* filepath) {
    assert(filepath->at(0) == '/');

    int dir_levels;
    stdlib::string** path = filepath->split('/', &dir_levels);

    fat_dir_entry* current_dir = &partition->root_dir;

    for (int i = 1; i < dir_levels; i++) {
        const char* fat_filename = filename_to_fat(*path[i]);
        current_dir = search_directory(partition, current_dir, fat_filename);
        if (current_dir == NULL) {
            log.e(fat_driver_tag, "(hd%d, gpt%d): Directory not found: %s",
                  partition->dev->disk_number, partition->partition, filepath->c_str());
            return NULL;
        }
    }

    return current_dir;
}

uint32_t get_file_size(FAT_partition* partition, uint32_t starting_cluster) {
    uint32_t file_size = 0, cluster = starting_cluster;
    do {
        file_size += partition->bpb->sectors_per_cluster * partition->bpb->bytes_per_sector;
        cluster = get_next_cluster(partition, cluster);
    } while (cluster < 0x0FFFFFF8);

    return file_size;
}


/* 
create_file:
    This function takes in a
*/
}  // namespace filesystem