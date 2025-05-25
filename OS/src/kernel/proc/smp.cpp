#include <types.h>
#include <limine/limine.h>
#include <kernel/kernel.h>

namespace kernel
{

    volatile limine::limine_smp_request smp_request = {LIMINE_SMP_REQUEST, 0};

    const char *kernel_scheduler_tag = "Scheduler";

    void smp_init(void)
    {
        kernel::initAPIC(smp_request.response->bsp_lapic_id);
        log::d(kernel_scheduler_tag, "%u CPU cores detected", smp_request.response->cpu_count);
        for (int i = 1; i < smp_request.response->cpu_count; i++)
        {
            cpu_jump_to(i, (void *)mt_begin);
            kernel::sleep(100);
        }
    }

    void mt_begin(limine::limine_smp_info *cpu_info)
    {
        kernel::initAPIC(cpu_info->lapic_id);
        for (;;)
        {
            asm("hlt");
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

        log::e(kernel_scheduler_tag, "Unable to find CPU #%u!", pid);
    }

    void panic()
    {
        asm("cli");
        asm("hlt");
    }

}