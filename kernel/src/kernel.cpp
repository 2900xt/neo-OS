#include <limine/limine.h>
#include <stdout.h>
#include <arch/amd64/io.h>
#include <mem.h>
#include <acpi/tables.h>
#include <vga/vga.h>
#include <math.h>
#include <vga/gfx.h>
#include <arch/amd64/smp.h>


#define IO_WAIT() outb(0x80, 0)

static volatile limine::limine_terminal_request terminal_request = {LIMINE_TERMINAL_REQUEST, 0};

static volatile limine::limine_smp_request smp_request = {LIMINE_SMP_REQUEST, 0};

void smp_init(void)
{
    klogf(LOG_IMPORTANT, "%d cores detected!\n", smp_request.response->cpu_count);
}

void cpu_jump_to(uint8_t pid, void *addr)
{
    int currentCPU = 0;
    limine::limine_smp_info *cpu;
    while (currentCPU <= smp_request.response->cpu_count)
    {
        cpu = smp_request.response->cpus[currentCPU];
        if (cpu->processor_id == pid)
        {
            cpu->goto_address = (limine::limine_goto_address)addr;
            return;
        }
        currentCPU++;
    }

    klogf(LOG_ERROR, "Unable to find CPU #%d!\n", currentCPU);
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

    for(int i = 1; i < smp_request.response->cpu_count; i++)
    {
        cpu_jump_to(i, (void*)mt_begin);
        for(int j = 0; j < 100000; j++)
        {
            IO_WAIT();
        }
    }

    bsp_done();
}
