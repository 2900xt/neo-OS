#include "drivers/vga/vga.h"
#include "kernel/vfs/file.h"
#include "kernel/mem/paging.h"
#include "stdlib/stdio.h"
#include "stdlib/string.h"
#include "stdlib/timer.h"
#include "types.h"
#include <limine/limine.h>
#include <stdlib/stdlib.h>
#include <kernel/x64/io.h>
#include <kernel/x64/intr/idt.h>
#include <kernel/mem/mem.h>
#include <kernel/smp.h>
#include <config.h>

extern "C" void __cxa_pure_virtual() { while (1); }

const char * kernel_tag = "NEO-OS Kernel";

void bsp_done(void)
{
    for (;;)
    {
        call_timers();
    }
}

// Entry point
void onTimer()
{
    Log.v(kernel_tag, "On Timer");
}

extern "C" void _start(void)
{
    std::tty_init();
    
    kernel::enableSSE();

    kernel::fillIDT();

    heapInit();

    kernel::initialize_page_allocator();

    Log.v("Kernel", "Starting Neo-OS:");
    
    fbuf_init();

    smp_init();

    kernel::load_drivers();

    image *icon = loadImage("/logo.nic");
    drawImage(icon, (fbuf_info->width - icon->w)/ 2, (fbuf_info->height - icon->h)/ 2);
    
    repaintScreen();
    bsp_done();
}


