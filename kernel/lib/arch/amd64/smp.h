#ifndef ACPI_SMP_H
#define ACPI_SMP_H

void smp_init(void);
void mt_begin(limine::limine_smp_info* cpu_info);

#endif ACPI_SMP_H