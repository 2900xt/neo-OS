#include <drivers/acpi/madt.h>
#include <kernel/x64/intr/apic.h>
#include <stdlib/stdio.h>

ACPI::MADT*              madt;
ACPI::LAPIC_ENTRY*       local_apics[32];
uint64_t                 lapicCount;
ACPI::IOAPIC_ENTRY*      ioapic;
uint32_t*                lapic;

namespace ACPI
{

void evaluateMADTEntry(ACPI::MADT_ENTRY_HDR* const tableEntry)
{
    switch (tableEntry->entryType)
    {
        case 0: //Processor local APIC
            local_apics[lapicCount] = (LAPIC_ENTRY*)tableEntry;

            Log.v(
            kernel_tag,
            "LAPIC found -> PID: %u APIC ID: %u",
            local_apics[lapicCount]->processor_id,
            local_apics[lapicCount]->apic_id
            );

            lapicCount++;
            break;
        case 1:
            ioapic = (IOAPIC_ENTRY*)tableEntry;

            Log.v(
            kernel_tag,
            "IOAPIC found -> addr 0x%x ID: %u",
            ioapic->ioapic_base,
            ioapic->ioapic_id
            );

            break;
        default:
            break;
    }
}

void parse_madt()
{
    madt = (ACPI::MADT*)ACPI::findACPITable("APIC");
    lapic = (uint32_t*)madt->LAPIC_addr;

    lapicCount = 0;
    int entryCount = 0;

    void* madtEnd = (void*)((uint8_t*)madt + madt->hdr.length);
    for(uint8_t* ptr = (uint8_t*)(&madt->ptr); ptr < madtEnd; ptr += ptr[1])
    {
        if(ptr[1] == 0 || entryCount >= 12)
            break;
        evaluateMADTEntry((ACPI::MADT_ENTRY_HDR*)ptr);
        entryCount++;
    }
}

    
}