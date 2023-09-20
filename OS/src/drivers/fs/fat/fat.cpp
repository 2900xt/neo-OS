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

static bios_param_block *read_bpb(DISK::rw_disk_t *device, int partition)
{
    uint32_t lba = DISK::get_gpt(device)->entries[partition].starting_lba;
    bios_param_block *data = (bios_param_block*)kernel::allocate_pages(1);
    mmap(data, data);
    DISK::read(device, lba, 1, data);
    return data;
}

static VFS::File* load_root(DISK::rw_disk_t *device, int partition)
{
    return NULL;
}

}