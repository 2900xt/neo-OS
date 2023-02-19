#include <types.h>
#include <limine/limine.h>
#include <stdlib/stdio.h>
#include <x64/intr/apic.h>

extern limine::limine_smp_request smp_request;

void mt_begin(limine::limine_smp_info* cpu_info)
{
    klogf(LOG_IMPORTANT, "[CPU%d] Online\n", cpu_info->processor_id);
    initAPIC(cpu_info->lapic_id);
    for(;;){
        asm("nop");
    }
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

    klogf(LOG_ERROR, "Unable to find CPU #%d!\n", pid);
}
