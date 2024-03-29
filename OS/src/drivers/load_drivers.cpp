#include "drivers/disk/disk_driver.h"
#include "drivers/fs/fat/fat.h"
#include "kernel/mem/paging.h"
#include "kernel/vfs/file.h"
#include "stdlib/stdio.h"
#include <drivers/pci/pci.h>
#include <drivers/disk/ahci/ahci.h>
#include <drivers/vga/fonts.h>

namespace kernel
{

void load_drivers()
{

    PCI::enumerate_pci();
    DISK::ahci_init();
    VFS::vfs_init();
    VGA::initialize_font();
}

}