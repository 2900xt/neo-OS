#pragma once
#include "drivers/disk/disk_driver.h"
#include "drivers/fs/fat/fat.h"

void testDiskDriver(disk::rw_disk_t* cur);
void testFATDriver(filesystem::FAT_partition* part);