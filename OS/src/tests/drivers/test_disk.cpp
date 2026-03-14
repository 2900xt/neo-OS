#include "drivers/disk/disk_driver.h"
#include "drivers/fs/fat/fat.h"
#include "kernel/mem/mem.h"
#include "kernel/mem/paging.h"
#include "stdlib/assert.h"
#include "kernel/io/log.h"
#include "stdlib/structures/string.h"

void testDiskDriver(disk::rw_disk_t* cur)
{
    uint64_t lba = 0;
    uint64_t num = 4;
    uint64_t* buf = (uint64_t*)kernel::allocate_pages(1);
    uint64_t* data = (uint64_t*)kernel::allocate_pages(1);
    mmap(buf, buf);
    mmap(data, data);

    buf[21] = 0xAA55AA55;

    disk::read(cur, lba, num, data);
    disk::write(cur, lba, num, buf);

    kernel::memset(buf, 0x1000, 0);

    disk::read(cur, lba, num, buf);

    assert(buf[21] == 0xAA55AA55);

    disk::write(cur, lba, num, data);
    log.v(kernel::kernel_tag, "DISK TEST SUCCESS");
}

void testFATDriver(filesystem::FAT_partition* part)
{
    stdlib::string path("/efi/boot/bootx64.efi");
    filesystem::fat_dir_entry* entry = filesystem::get_file_entry(part, &path);
    uint32_t cluster = entry->first_cluster_l | (entry->first_cluster_h << 16);
    uint64_t* data = (uint64_t*)kernel::allocate_pages(25);
    mmap(data, data);

    data[1067] = 0xAA55AA55;
    filesystem::write_cluster_chain(part, data, entry->file_size);
    kernel::memset(data, 0x1000, 0);

    filesystem::read_cluster_chain(part, data, cluster);
    assert(data[1067] == 0xAA55AA55);

    log.v(kernel::kernel_tag, "FAT DRIVER TEST SUCCESS");
}