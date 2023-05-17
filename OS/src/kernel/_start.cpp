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

image *logo;
int dx, dy, x, y;
void logoAnimation()
{
    drawImage(logo, x, y);

    if(x < 5 || (x + logo->w) > (fbuf_info->width - 10))
    {
        dx *= -1;
    }

    if(y < 5 || (y + logo->h) > (fbuf_info->height - 10))
    {
        dy *= -1;
    }

    x += dx;
    y += dy;

    repaintScreen();
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

    logo = loadImage("/logo.nic");
    x = (fbuf_info->width - logo->w)/ 2;
    y = (fbuf_info->height - logo->h)/ 2;
    dx = 5;
    dy = 5;

    register_timer(&logoAnimation, 50);
    
    repaintScreen();
    bsp_done();
}


