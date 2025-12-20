#include "drivers/disk/ahci/ahci.h"
#include "drivers/fs/gpt.h"
#include "drivers/fs/gpt.h"
#include "kernel/mem/mem.h"
#include "kernel/mem/paging.h"
#include "limine/limine.h"
#include "stdlib/assert.h"
#include <stdlib/structures/string.h>


#include <types.h>
#include <drivers/fs/fat/fat.h>
#include <drivers/disk/disk_driver.h>
#include <kernel/vfs/file.h>
#include <kernel/io/log.h>

namespace filesystem
{

    static const char *fat_driver_tag = "FAT Driver";
    static constexpr int MAX_FILENAME_LENGTH = 11;

    static bios_param_block *read_bpb(disk::rw_disk_t *device, int partition)
    {
        uint32_t lba = disk::get_gpt(device)->entries[partition].starting_lba;
        bios_param_block *data = (bios_param_block *)kernel::allocate_pages(1);
        mmap(data, data);
        disk::read(device, lba, 1, data);
        return data;
    }

    static fsinfo_fat32 *read_fsinfo(disk::rw_disk_t *device, bios_param_block *bpb, int partition)
    {
        uint32_t lba = disk::get_gpt(device)->entries[partition].starting_lba + bpb->fs_info_sector;
        fsinfo_fat32 *data = (fsinfo_fat32 *)kernel::allocate_pages(1);
        mmap(data, data);
        disk::read(device, lba, 1, data);
        return data;
    }

    static void read_fat(FAT_partition *partition)
    {
        uint64_t page_count = (partition->fat_size * partition->bpb->bytes_per_sector) / 0x1000 + 1;
        uint32_t lba = partition->first_fat_sector;
        partition->fat = (uint32_t *)kernel::allocate_pages(page_count);
        kernel::map_pages((uint64_t)partition->fat, (uint64_t)partition->fat, page_count);
        disk::read(partition->dev, lba, partition->fat_size, partition->fat);
    }

    static uint32_t get_next_cluster(filesystem::FAT_partition *partition, int current_cluster)
    {
        return partition->fat[current_cluster] & ~0xF0000000;
    }

    // Make sure to free the allocated pages after using them
    void *read_cluster_chain(filesystem::FAT_partition *partition, fat_dir_entry *file_entry)
    {
        int page_count = file_entry->file_size / 0x1000 + 1;
        void *buffer = kernel::allocate_pages(page_count);
        uint8_t *ptr = (uint8_t *)buffer;

        kernel::map_pages((uint64_t)buffer, (uint64_t)buffer, page_count);

        uint32_t cluster = ((uint32_t)file_entry->first_cluster_h << 16) | (file_entry->first_cluster_l);
        do
        {
            if (cluster == 0x0FFFFFF7)
            {
                log.e(fat_driver_tag, "(hd%d, gpt%d): Encountered bad sector while reading %s", partition->dev->disk_number, partition->partition, file_entry->dir_name);
                kernel::free_pages(buffer);
                return NULL;
            }

            uint32_t lba = partition->first_data_sector + (cluster - 2) * partition->bpb->sectors_per_cluster;
            disk::read(partition->dev, lba, partition->bpb->sectors_per_cluster, ptr);

            ptr += partition->bpb->bytes_per_sector * partition->bpb->sectors_per_cluster;
            cluster = get_next_cluster(partition, cluster);

        } while (cluster < 0x0FFFFFF8);

        return buffer;
    }

    static void print_file_info(fat_dir_entry *file)
    {
        log.d(fat_driver_tag, "%s | sz: 0x%x | cluster: 0x%x | attrib: 0x%x", file->dir_name, file->file_size, file->first_cluster_l | ((uint32_t)file->first_cluster_h << 16), file->dir_attrib);
    }

    static fat_dir_entry *copy_dir_entry(fat_dir_entry *entry)
    {
        fat_dir_entry *copy = new fat_dir_entry;
        kernel::memcpy(copy, entry, sizeof(fat_dir_entry));
        return copy;
    }

    void print_directory_contents(FAT_partition *partition, fat_dir_entry *directory)
    {
        void *buffer = read_cluster_chain(partition, directory);
        fat_dir_entry *current_entry = (fat_dir_entry *)buffer;
        while (true)
        {
            if (current_entry->dir_name[0] == 0x00)
                break;

            if (!((uint8_t)current_entry->dir_name[0] == 0xE5 || current_entry->dir_attrib == LONG_NAME))
            {
                if (current_entry->file_size > 1000000)
                {
                    log.d(fat_driver_tag, "%dK\t%s", current_entry->file_size / 1000000, current_entry->dir_name);
                }
                else if (current_entry->file_size > 1000)
                {
                    log.d(fat_driver_tag, "%dK\t%s", current_entry->file_size / 1000, current_entry->dir_name);
                }
                else
                {
                    log.d(fat_driver_tag, "%dB\t%s", current_entry->file_size, current_entry->dir_name);
                }
            }
            current_entry++;
        }

        kernel::free_pages(buffer);
    }

    // Don't forget to kfree the dir entry
    // Reads a directory into memory and searches for the requested entry
    static fat_dir_entry *search_directory(FAT_partition *partition, fat_dir_entry *directory, const char *filename)
    {
        void *buffer = read_cluster_chain(partition, directory);
        fat_dir_entry *current_entry = (fat_dir_entry *)buffer;

        while (true)
        {
            if (current_entry->dir_name[0] == 0x00)
                break;

            if ((uint8_t)current_entry->dir_name[0] == 0xE5 || current_entry->dir_attrib == LONG_NAME)
            {
                current_entry++;
                continue;
            }

            if (stdlib::strcmp(filename, current_entry->dir_name, 11))
            {
                fat_dir_entry *copy_of_entry = copy_dir_entry(current_entry);
                kernel::free_pages(buffer);
                return copy_of_entry;
            }

            current_entry++;
        }

        kernel::free_pages(buffer);
        return NULL;
    }

    // Don't forget to kfree the output
    const char *filename_to_fat(stdlib::string &formal_filename)
    {
        char *fat_filename = new char[MAX_FILENAME_LENGTH];
        kernel::memset(fat_filename, MAX_FILENAME_LENGTH, ' ');

        int srcIndex = 0;

        for (int i = 0; i < MAX_FILENAME_LENGTH; i++)
        {
            char srcChar = formal_filename.c_str()[srcIndex];
            if (srcChar == '\0')
            {
                break;
            }

            if (srcChar == '.')
            {
                i = 7;
                srcIndex++;
                continue;
            }

            fat_filename[i] = stdlib::toUpper(srcChar);
            srcIndex++;
        }

        return fat_filename;
    }

    fat_dir_entry *get_file_entry(FAT_partition *partition, stdlib::string *filepath)
    {
        assert(filepath->at(0) == '/');

        int dir_levels;
        stdlib::string **path = filepath->split('/', &dir_levels);

        fat_dir_entry *current_dir = &partition->root_dir;

        for (int i = 1; i < dir_levels; i++)
        {
            const char *fat_filename = filename_to_fat(*path[i]);
            current_dir = search_directory(partition, current_dir, fat_filename);
            if (current_dir == NULL)
            {
                log.e(fat_driver_tag, "(hd%d, gpt%d): Directory not found: %s", partition->dev->disk_number, partition->partition, filepath->c_str());
                return NULL;
            }
        }

        return current_dir;
    }

    filesystem::FAT_partition *mount_part(disk::rw_disk_t *device, int part_num, kernel::file_handle *folder)
    {
        bios_param_block *bpb = read_bpb(device, part_num);

        if (bpb->magic_number != 0xAA55)
        {
            log.e(fat_driver_tag, "(hd%d, gpt%d): Invalid BPB Magic Number: 0x%x", device->disk_number, part_num, bpb->magic_number);
            return NULL;
        }

        fsinfo_fat32 *fsinfo = read_fsinfo(device, bpb, part_num);

        if (
            fsinfo->signature_3 != FSINFO_SIGNATURE_3 ||
            fsinfo->signature_2 != FSINFO_SIGNATURE_2 ||
            fsinfo->signature_1 != FSINFO_SIGNATURE_1)
        {
            log.e(fat_driver_tag, "(hd%d, gpt%d): Invalid FSINFO Magic Number: (0x%x, 0x%x, 0x%x)", device->disk_number, part_num, fsinfo->signature_1, fsinfo->signature_2, fsinfo->signature_3);
            return NULL;
        }

        log.v(fat_driver_tag, "(hd%d, gpt%d): Available Clusters: 0x%x\tFirst Cluster: 0x%x", device->disk_number, part_num, fsinfo->free_cluster_count, fsinfo->cluster_search_start);

        filesystem::FAT_partition *partition = new filesystem::FAT_partition;
        partition->bpb = bpb;
        partition->fsinfo = fsinfo;

        partition->dev = device;
        partition->partition = part_num;

        partition->first_sector = disk::get_gpt(partition->dev)->entries[partition->partition].starting_lba;

        partition->total_sectors = bpb->total_sector_count_32;
        partition->fat_size = bpb->fat_size_32;
        partition->first_fat_sector = partition->first_sector + bpb->reserved_sectors;
        partition->first_data_sector = partition->first_fat_sector + (partition->fat_size * bpb->num_fats);
        partition->total_clusters = partition->total_sectors / bpb->sectors_per_cluster;

        read_fat(partition);

        if (partition->fat == NULL)
        {
            log.e(fat_driver_tag, "(hd%d, gpt%d): Error trying to read FAT", device->disk_number, part_num);
        }

        // Get the root directory information

        uint32_t cluster = bpb->root_dir_cluster;
        do
        {
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

}