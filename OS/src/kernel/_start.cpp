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

extern "C" void __cxa_pure_virtual() { while (1); }
extern uint32_t *g_framebuffer2;
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
    std::tty_init();
    std::klogf("Loading Kernel NEO...\n\n");
    
    kernel::enableSSE();

    kernel::fillIDT();

    heapInit();

    kernel::initialize_page_allocator();
    
    fbuf_init();

    smp_init();

    kernel::load_drivers();

    VFS::File *fp = VFS::open("logo.tga");
    void *buf = kernel::allocate_pages(fp->file_size / 0x1000 + 1);
    VFS::read(fp, buf);
    image * img = tga_parse(buf, fp->file_size);
    
    repaintScreen();
    bsp_done();
}
