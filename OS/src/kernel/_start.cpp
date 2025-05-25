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
        for (;;)
        {
            kernel::printf("HELLO\n");
            kernel::update_terminal();
        }
    }

    extern "C" void _start(void)
    {
        kernel::tty_init();

        kernel::enableSSE();

        kernel::fillIDT();

        kernel::heapInit();

        kernel::initialize_page_allocator();

        log::v(kernel_tag, "Starting Neo-OS:");

        vga::fbuf_init();

        kernel::smp_init();

        kernel::load_drivers();
        kernel_stdout = new kernel::stream();

        kernel::printf("HELLO\n");

        bsp_done();
    }
}