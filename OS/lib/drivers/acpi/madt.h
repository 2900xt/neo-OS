#pragma once

#include <types.h>
#include <drivers/acpi/sdt.h>

namespace ACPI 
{

struct MADT_ENTRY_HDR
{
    uint8_t entryType;
    uint8_t entryLength;
}__attribute__ ((packed));


struct MADT
{
    SDT_HEADER         hdr;
    uint32_t                LAPIC_addr;
    uint32_t                LAPIC_flags;
    MADT_ENTRY_HDR     ptr;
}__attribute__ ((packed));

void parse_madt();

}