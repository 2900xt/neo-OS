#include "drivers/disk/disk_driver.h"
#include "drivers/fs/fat/fat.h"
#include "kernel/vfs/file.h"
#include "stdlib/stdio.h"
#include <drivers/pci/pci.h>
#include <drivers/disk/ahci/ahci.h>

namespace kernel
{

void load_drivers()
{
    PCI::enumerate_pci();
    DISK::ahci_init();

    //Try to load the BPB from the first hard drive

    DISK::rw_disk_t *hd0 = (DISK::rw_disk_t*)VFS::get_root()->get_subdir("dev")->get_subdir("hd0")->file_data;
    FS::FATPartition *esp = new FS::FATPartition(hd0, 0);
    void *file = esp->read_file("test/lol/test.txt");
    std::klogf("%s\n", file);
}

}