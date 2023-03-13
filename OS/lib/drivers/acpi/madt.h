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


struct LAPIC_NMI
{
    ACPI::MADT_ENTRY_HDR hdr;
    uint8_t             processor_id;
    uint16_t            flags;
    uint8_t             local_int;
}__attribute__ ((packed));


struct LAPIC_ENTRY
{
    ACPI::MADT_ENTRY_HDR hdr;
    uint8_t             processor_id;
    uint8_t             apic_id;
    uint32_t            flags;
}__attribute__ ((packed));

struct IOAPIC_ENTRY
{
    ACPI::MADT_ENTRY_HDR hdr;
    uint8_t             ioapic_id;
    uint8_t             reserved;
    uint32_t            ioapic_base;
    uint32_t            system_interrupt_base;
}__attribute__ ((packed));

struct IOAPIC_INT_SOURCE
{
    ACPI::MADT_ENTRY_HDR hdr;
    uint8_t             bus_source;
    uint8_t             irq_source;
    uint32_t            global_interrupt;
    uint16_t            flags;
}__attribute__ ((packed));

struct IOAPIC_NMI_SOURCE
{
    ACPI::MADT_ENTRY_HDR hdr;
    uint8_t             source;
    uint16_t            flags;
    uint32_t            global_interrupt;
}__attribute__ ((packed));

void parse_madt();

}