#include <drivers/vga/fonts.h>
#include <drivers/vga/vga.h>
#include <stdlib/stdlib.h>
#include <limine/limine.h>
#include <kernel/kernel.h>

namespace kernel
{
    const char *kernel_tag = "Kernel";
    extern stream *kernel_stdout, *kernel_stdin;

    void bsp_done(void)
    {
        while (true)
        {
            asm volatile("hlt");
        }

        __builtin_unreachable();
    }

    extern "C" void _start(void)
    {
        kernel::tty_init();
        kernel::enableSSE();
        kernel::fillIDT();
        kernel::heapInit();

        log::v(kernel_tag, "Starting Neo-OS");

        kernel::initialize_page_allocator();

        vga::fbuf_init();

        kernel::load_drivers();

        kernel::smp_init();

        bsp_done();
    }
}