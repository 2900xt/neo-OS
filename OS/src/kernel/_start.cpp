#include "drivers/vga/vga.h"
#include "kernel/vfs/file.h"
#include "kernel/mem/paging.h"
#include "stdlib/stdio.h"
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

    bsp_done();
}
