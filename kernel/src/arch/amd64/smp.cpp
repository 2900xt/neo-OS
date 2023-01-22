#include <types.h>
#include <limine/limine.h>
#include <stdout.h>
#include <acpi/tables.h>


void mt_begin(limine::limine_smp_info* cpu_info)
{
    klogf(LOG_IMPORTANT, "[CPU%d] Online\n", cpu_info->processor_id);
    initAPIC(cpu_info->lapic_id);
    asm("cli");
    for(;;){
        asm("nop");
    }
}