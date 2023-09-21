#include "drivers/vga/fonts.h"
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

const char * kernel_tag = "Kernel";

void bsp_done(void)
{
    for (;;)
    {
        call_timers();
    }
}



extern "C" void _start(void)
{
    std::tty_init();
    
    kernel::enableSSE();

    kernel::fillIDT();

    heapInit();

    kernel::initialize_page_allocator();

    Log.v(kernel_tag, "Starting Neo-OS:");
    
    VGA::fbuf_init();

    kernel::smp_init();

    kernel::load_drivers();

    std::string path {"/bin/font.psf"};
    VFS::File* font_file = new VFS::File;
    VFS::open(font_file, &path);
    uint8_t* buffer = (uint8_t*)VFS::read(font_file);

    PSF_header_t* hdr = (PSF_header_t*)buffer;
    uint8_t* bitmap = buffer + hdr->header_sz;

    VGA::Color fg {255, 255, 255};
    VGA::Color bg {50, 50, 50};

    uint8_t* glyph = ('A' * hdr->glyph_size) + bitmap;

    for(int y = 0; y < hdr->height; y++)
    {
        for(int x = 0; x < hdr->width; x++)
        {
            if(*glyph & (1 << x))
            {
                VGA::putpixel(x, y, fg);
            }
            else 
            {
                VGA::putpixel(x, y, bg);
            }
        }

        glyph += hdr->width / 8 + 1;
        if(hdr->width % 8 == 0)
        {
            glyph--;
        }
    }

    VGA::repaintScreen();

    bsp_done();
}