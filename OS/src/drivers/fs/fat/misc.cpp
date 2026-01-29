#include <drivers/disk/disk_driver.h>
#include <drivers/fs/fat/fat.h>
#include <kernel/io/log.h>
#include <kernel/vfs/file.h>
#include <stdlib/structures/string.h>
#include <types.h>

#include "kernel/mem/mem.h"
#include "kernel/mem/paging.h"

namespace filesystem {

static constexpr int MAX_FILENAME_LENGTH = 11;
static const char* fat_driver_tag = "FAT Driver";

// Don't forget to kfree the output
const char* filename_to_fat(stdlib::string& formal_filename) {
    char* fat_filename = new char[MAX_FILENAME_LENGTH];
    kernel::memset(fat_filename, MAX_FILENAME_LENGTH, ' ');

    int srcIndex = 0;

    for (int i = 0; i < MAX_FILENAME_LENGTH; i++) {
        char srcChar = formal_filename.c_str()[srcIndex];
        if (srcChar == '\0') {
            break;
        }

        if (srcChar == '.') {
            i = 7;
            srcIndex++;
            continue;
        }

        fat_filename[i] = stdlib::toUpper(srcChar);
        srcIndex++;
    }

    return fat_filename;
}

void print_file_info(fat_dir_entry* file) {
    log.d(fat_driver_tag, "%s | sz: 0x%x | cluster: 0x%x | attrib: 0x%x", file->dir_name,
          file->file_size, file->first_cluster_l | ((uint32_t)file->first_cluster_h << 16),
          file->dir_attrib);
}

fat_dir_entry* copy_dir_entry(fat_dir_entry* entry) {
    fat_dir_entry* copy = new fat_dir_entry;
    kernel::memcpy(copy, entry, sizeof(fat_dir_entry));
    return copy;
}

void print_directory_contents(FAT_partition* partition, fat_dir_entry* directory) {
    void* buffer = read_cluster_chain(partition, directory);
    fat_dir_entry* current_entry = (fat_dir_entry*)buffer;
    while (true) {
        if (current_entry->dir_name[0] == 0x00) break;

        if (!((uint8_t)current_entry->dir_name[0] == 0xE5 ||
              current_entry->dir_attrib == LONG_NAME)) {
            if (current_entry->file_size > 1000000) {
                log.d(fat_driver_tag, "%dK\t%s", current_entry->file_size / 1000000,
                      current_entry->dir_name);
            } else if (current_entry->file_size > 1000) {
                log.d(fat_driver_tag, "%dK\t%s", current_entry->file_size / 1000,
                      current_entry->dir_name);
            } else {
                log.d(fat_driver_tag, "%dB\t%s", current_entry->file_size, current_entry->dir_name);
            }
        }
        current_entry++;
    }

    kernel::free_pages(buffer);
}
}  // namespace filesystem