#include <limine/limine.h>
#include <stdout.h>
#include <arch/amd64/io.h>
#include <mem.h>
#include <acpi/tables.h>
#include <vga/vga.h>
#include <math.h>
#include <vga/gfx.h>
#include <arch/amd64/smp.h>
#include <proc.h>
#include <drivers/pci.h>

const void* _Unwind_Resume;

static volatile limine::limine_terminal_request terminal_request = {LIMINE_TERMINAL_REQUEST, 0};

volatile limine::limine_smp_request smp_request = {LIMINE_SMP_REQUEST, 0};

process_t* init_proc;

void smp_init(void)
{
    klogf(LOG_IMPORTANT, "%d cores detected!\n", smp_request.response->cpu_count);
}


void bsp_done(void)
{
    for (;;);
}

extern double s_since_boot;

// Entry point ->

extern "C" void _start(void)
{

    enableSSE();

    s_since_boot = 0.0;

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

    for(int i = 1; i < smp_request.response->cpu_count; i++)
    {
        cpu_jump_to(i, (void*)mt_begin);
        sleep(100);
    }

    bsp_done();
}


