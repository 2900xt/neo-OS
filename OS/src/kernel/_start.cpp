#include <limine/limine.h>
#include <kernel/io/log.h>
#include <kernel/x64/io.h>
#include <kernel/x64/intr/idt.h>
#include <kernel/mem/mem.h>
#include <kernel/mem/paging.h>
#include <drivers/vga/vga.h>
#include <drivers/pci/pci.h>
#include <drivers/disk/ahci/ahci.h>
#include <drivers/vga/fonts.h>
#include <drivers/network/rtl8139.h>
#include <kernel/vfs/file.h>
#include <kernel/smp/smp.h>
#include <kernel/shell/shell.h>
#include <kernel/x64/intr/apic.h>
#include <stdlib/timer.h>
#include <kernel/io/scan.h>
#include <kernel/io/log.h>
#include <stdlib/structures/hashmap.h>
#include <stdlib/assert.h>

namespace kernel
{
    const char *kernel_tag = "Kernel";

    void test_hashmap()
    {
        stdlib::HashMap<stdlib::string, int> map(stdlib::hash_string, 10);
        map.put("one", 1);
        map.put("two", 2);
        map.put("three", 3);

        assert(map.get("one") == 1 && "HashMap get failed for key 'one'");
        assert(map.get("two") == 2 && "HashMap get failed for key 'two'");
        assert(map.get("three") == 3 && "HashMap get failed for key 'three'");

        map.remove("two");
        assert(!map.contains("two") && "HashMap remove failed for key 'two'");
    }

    extern "C" void _start(void)
    {
        kernel::log_init();
        kernel::enable_sse();
        kernel::fill_idt();
        kernel::heapInit();
        kernel::initialize_page_allocator();
        vga::framebuffer_init();

        pci::enumerate_pci();
        disk::ahci_init();
        kernel::vfs_init();
        vga::initialize_font();
        network::rtl8139_init();
        kernel::smp_init();
        
        test_hashmap();
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