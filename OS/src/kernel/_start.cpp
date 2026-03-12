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
#include <kernel/proc/smp.h>
#include <kernel/shell/shell.h>
#include <kernel/x64/intr/apic.h>
#include <stdlib/timer.h>
#include <kernel/io/scan.h>
#include <kernel/io/log.h>
#include <stdlib/assert.h>
#include <drivers/fs/fat/fat.h>

namespace kernel
{
    const char *kernel_tag = "Kernel";

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

        // Simple write_cluster_chain test
        {
            filesystem::FAT_partition* part = kernel::get_root_partition();
            stdlib::string path("/test.txt");
            filesystem::fat_dir_entry* entry = filesystem::get_file_entry(part, &path);
            if (entry) {
                char buf[] = "Hello from write test!\n";
                int result = filesystem::write_cluster_chain(part, entry, buf, sizeof(buf));
                if (result >= 0) {
                    log.d(kernel_tag, "write_cluster_chain test: wrote to starting cluster %d", result);

                    // Read it back to verify
                    void* readback = filesystem::read_cluster_chain(part, entry);
                    if (readback) {
                        char* data = (char*)readback;
                        if (stdlib::strcmp(data, buf, sizeof(buf))) {
                            log.d(kernel_tag, "write_cluster_chain test PASSED: readback matches");
                        } else {
                            log.e(kernel_tag, "write_cluster_chain test FAILED: readback mismatch");
                            log.e(kernel_tag, "  expected: \"%s\"", buf);
                            log.e(kernel_tag, "  got:      \"%s\"", data);
                        }
                        kernel::free_pages(readback);
                    } else {
                        log.e(kernel_tag, "write_cluster_chain test FAILED: could not read back");
                    }
                } else {
                    log.e(kernel_tag, "write_cluster_chain test failed: not enough clusters");
                }
            } else {
                log.w(kernel_tag, "write_cluster_chain test skipped: TEST.TXT not found");
            }
        }

        while (true)
        {
            asm volatile("hlt");
        }

        __builtin_unreachable();
    }
}