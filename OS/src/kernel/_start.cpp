#include "drivers/vga/vga.h"
#include "kernel/vfs/file.h"
#include "kernel/mem/paging.h"
#include "stdlib/stdio.h"
#include "types.h"
#include <limine/limine.h>
#include <stdlib/stdlib.h>
#include <kernel/x64/io.h>
#include <kernel/x64/intr/idt.h>
#include <kernel/mem/mem.h>
#include <kernel/x64/intr/apic.h>
#include <drivers/pci/pci.h>
#include <drivers/ahci/ahci.h>
#include <kernel/smp.h>
#include <kernel/proc.h>

void bsp_done(void)
{
    for (;;)
    {
        asm volatile ("hlt");
    }
}

// Entry point

extern "C" void _start(void)
{
    kernel::enableSSE();

    std::tty_init();
    
    fbuf_init();

    std::klogf("Loading NEO-OS 0.01 Alpha...\n\n");

    kernel::fillIDT();

    heapInit();

    kernel::initialize_page_allocator();
    
    smp_init();

    VFS::vfs_init();

    PCI::enumerate_pci();

    AHCI::ahci_init();

    VFS::file_t *hd0 = VFS::get_root()->get_subdir("dev")->get_subdir("hd0");
    AHCI::AHCIDevice *dev = (AHCI::AHCIDevice*)hd0->file_data;
    uint8_t* buffer = (uint8_t*)kernel::allocate_pages(1);
    dev->read_sectors(0, 4, buffer);

    for(int i = 0; i < 512; i++)
    {
        std::klogf("%x ", buffer[i]);
    }

    bsp_done();
}
