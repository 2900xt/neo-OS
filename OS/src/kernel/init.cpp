#include "kernel/vfs/file.h"
#include "stdlib/stdio.h"
#include <limine/limine.h>
#include <stdlib/stdlib.h>
#include <kernel/x64/io.h>
#include <kernel/x64/intr/idt.h>
#include <kernel/mem.h>
#include <kernel/x64/intr/apic.h>
#include <drivers/pci/pci.h>
#include <drivers/ahci/ahci.h>
#include <kernel/smp.h>

const void* _Unwind_Resume;

void bsp_done(void)
{
    for (;;);
}

// Entry point

extern "C" void _start(void)
{
    AMD64::enableSSE();

    std::tty_init();

    std::klogf("Loading NEO-OS...\n");

    AMD64::fillIDT();

    heapInit(0x100000, 0x100000, 0x100);

    smp_init();

    VFS::vfs_init();

    PCI::enumerate_pci();

    AHCI::ahci_init();

    bsp_done();
}
