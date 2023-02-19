#pragma once

#include <types.h>
#include <drivers/acpi/sdt.h>

struct ACPI_MADT_ENTRY_HDR
{
    uint8_t entryType;
    uint8_t entryLength;
}__attribute__ ((packed));


struct ACPI_MADT
{
    ACPI_SDT_HEADER         hdr;
    uint32_t                LAPIC_addr;
    uint32_t                LAPIC_flags;
    ACPI_MADT_ENTRY_HDR     ptr;
}__attribute__ ((packed));