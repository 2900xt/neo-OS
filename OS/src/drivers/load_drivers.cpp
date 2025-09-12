#include "drivers/disk/disk_driver.h"
#include "drivers/fs/fat/fat.h"
#include "kernel/mem/paging.h"
#include "kernel/vfs/file.h"
#include <kernel/kernel.h>
#include <drivers/pci/pci.h>
#include <drivers/disk/ahci/ahci.h>
#include <drivers/vga/fonts.h>
#include <drivers/network/rtl8139.h>
#include <drivers/ps2/ps2.h>
#include <drivers/rtc/rtc.h>

namespace kernel
{

    void load_drivers()
    {
        pci::enumerate_pci();
        disk::ahci_init();
        kernel::vfs_init();
        vga::initialize_font();
        rtc::rtc_init();
        //ps2::mouse_init();
        network::rtl8139_init();
    }

}