#include "drivers/ahci/ahci.h"
#include <limine/limine.h>
#include <stdlib/stdlib.h>
#include <drivers/acpi/sdt.h>
#include <vga/vga.h>
#include <x64/intr/idt.h>
#include <x64/io.h>
#include <x64/intr/apic.h>
#include <drivers/acpi/mcfg.h>
#include <drivers/pci/pci.h>
#include <hal/smp.h>

const void* _Unwind_Resume;

static volatile limine::limine_terminal_request terminal_request = {LIMINE_TERMINAL_REQUEST, 0};

volatile limine::limine_smp_request smp_request = {LIMINE_SMP_REQUEST, 0};

void smp_init(void)
{
    klogf(LOG_IMPORTANT, "%d cores detected!\n", smp_request.response->cpu_count);
}


void bsp_done(void)
{
    for (;;);
}


// Entry point ->

extern "C" void _start(void)
{

    enableSSE();

    // Check if we got a console

    if (terminal_request.response == NULL || terminal_request.response->terminal_count == 0)
    {
        bsp_done();
    }

    // Set global variables

    write = terminal_request.response->write;
    console = terminal_request.response->terminals[0];

    fillIDT();

    heapInit(0x100000, 0x100000, 0x100);

    initAPIC(smp_request.response->bsp_lapic_id);

    smp_init();

    enumerate_pci((ACPI_MCFG_HDR*)findACPITable("MCFG"));

    AHCI::ahci_init();

    for(int i = 1; i < smp_request.response->cpu_count; i++)
    {
        cpu_jump_to(i, (void*)mt_begin);
        sleep(100);
    }

    bsp_done();
}


