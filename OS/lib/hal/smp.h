#ifndef ACPI_SMP_H
#define ACPI_SMP_H

#include <limine/limine.h>
#include <types.h>

void smp_init(void);
void mt_begin(limine::limine_smp_info* cpu_info);

void cpu_jump_to(uint8_t pid, void *addr);

#endif //ACPI_SMP_H