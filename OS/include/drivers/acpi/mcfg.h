#pragma once
#include <types.h>
#include <drivers/acpi/sdt.h>

namespace ACPI 
{

struct MCFG_HDR
{
    SDT_HEADER hdr;
    uint64_t reserved;
}__attribute__((packed));

}