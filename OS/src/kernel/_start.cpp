#include <drivers/vga/fonts.h>
#include <drivers/vga/vga.h>
#include <stdlib/stdlib.h>
#include <limine/limine.h>
#include <kernel/kernel.h>
#include <drivers/ps2/ps2.h>

namespace kernel
{
    const char *kernel_tag = "Kernel";
    extern stream *kernel_stdout, *kernel_stdin;

    extern "C" void _start(void)
    {
        kernel::tty_init();
        kernel::enableSSE();
        kernel::fillIDT();
        kernel::heapInit();
        kernel::initialize_page_allocator();
        vga::framebuffer_init();
        kernel::load_drivers();
        kernel::smp_init();
        
        kernel::terminal_init();
        kernel::login_init();

        while (true)
        {
            kernel::sleep(5);
            stdlib::call_timers();
            kernel::pollNextChar();

            if (vga::g_framebuffer_dirty)
                vga::repaintScreen();
            asm volatile("hlt");
        }

        __builtin_unreachable();
    }
}