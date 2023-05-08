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
}

}