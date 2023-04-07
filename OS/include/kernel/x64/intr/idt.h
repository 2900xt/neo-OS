#include <types.h>

#pragma once

namespace kernel
{


struct interruptFrame
{
    uint64_t IP;
    uint64_t CS;
    uint64_t flags;
    uint64_t SP;
    uint64_t BP;
}__attribute__ ((packed));

void remapPIC(int offset1, int offset2);
void fillIDT(void);
void register_isr(uint8_t vector, __attribute__((interrupt)) void (*isr)(interruptFrame*));

}
