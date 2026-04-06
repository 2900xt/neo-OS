#include "drivers/disk/disk_driver.h"
#include "drivers/fs/fat/fat.h"
#include "kernel/mem/mem.h"
#include "kernel/mem/paging.h"
#include "stdlib/assert.h"
#include "kernel/io/log.h"

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

