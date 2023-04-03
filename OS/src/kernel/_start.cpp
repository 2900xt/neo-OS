#include "drivers/ahci/ahci.h"
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
#include <kernel/smp.h>
#include <config.h>

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
    
    std::klogf("Loading NEO-OS\n\n");

    kernel::fillIDT();

    heapInit();

    kernel::initialize_page_allocator();
    
    fbuf_init();

    smp_init();

    VFS::vfs_init();

    kernel::load_drivers();

    AHCI::AHCIDevice* hd0 = (AHCI::AHCIDevice*)VFS::get_root()->get_subdir("dev")->get_subdir("hd0")->file_data;
    uint8_t* buf = (uint8_t*)kernel::allocate_pages(1);
    int sts = hd0->read(0, 1, buf);

    for(int i = 0; i < 512; i++)
    {
        std::klogf(" %x", buf[i]);
    }


    std::klogf("\nStatus: %d\n", sts);

    bsp_done();
}
