#pragma once

#include <limine/limine.h>
#include <types.h>
#include <stdlib/structures/string.h>

namespace kernel
{

    void smp_init(void);
    void mt_begin(limine::limine_smp_info *cpu_info);

    void cpu_jump_to(uint8_t pid, void *addr);
    void smp_init(void);

    void panic(stdlib::string message);

}