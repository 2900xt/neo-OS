#include <drivers/vga/fonts.h>
#include <drivers/vga/vga.h>
#include <stdlib/stdlib.h>
#include <limine/limine.h>
#include <kernel/kernel.h>

namespace kernel
{
    const char *kernel_tag = "Kernel";
    extern stream *kernel_stdout, *kernel_stdin;

    void splash_screen(void)
    {
        File logo_file;
        stdlib::string logo_path = "/bin/logo.nic";
        kernel::open(&logo_file, &logo_path);

        vga::nic_image *logo = (vga::nic_image *)kernel::read(&logo_file);
        vga::drawImage(logo, 0, 0, 150, 150);

        vga::putstring("Welcome to neo OS!", 0, 155);
        vga::repaintScreen();
    }

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
        kernel::splash_screen();

        while (true)
        {
            asm volatile("hlt");
        }

        __builtin_unreachable();
    }
}