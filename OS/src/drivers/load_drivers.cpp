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

    DISK::AHCIDevice *hd0 = (DISK::AHCIDevice*)VFS::get_root()->get_subdir("dev")->get_subdir("hd0")->file_data;
    FS::FATPartition *esp = new FS::FATPartition(hd0, 0);
    esp->create_file("test/hello.txt");
    void *file = esp->open_file("test/lol/test.txt");
    std::klogf("%s\n", file);
}

}