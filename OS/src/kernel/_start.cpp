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
#include <kernel/proc/stream.h>

const char * kernel_tag = "Kernel";

extern stream *kernel_stdout, *kernel_stdin;

int terminal_x = 0, terminal_y = 0;
void keyboardHandler();
void mouseINIT(void);

void bsp_done(void)
{
    for (;;)
    {
        if(kernel_stdout->ack_update)
        {
            std::string *data = stream_read(kernel_stdout);
            VGA::putstring(terminal_x, terminal_y, data->c_str());
            VGA::repaintScreen();
            terminal_x += data->length() * 8;
        }
        keyboardHandler();
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
    mouseINIT();

    kernel_stdout = new stream;

    bsp_done();
}