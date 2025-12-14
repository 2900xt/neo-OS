#include <drivers/acpi/madt.h>
#include <kernel/x64/intr/apic.h>


acpi::MADT *madt;
acpi::LAPIC_ENTRY *local_apics[32];
uint64_t lapicCount;
acpi::IOAPIC_ENTRY *ioapic;
uint32_t *lapic;

namespace acpi
{
    void evaluateMADTEntry(acpi::MADT_ENTRY_HDR *const tableEntry)
    {
        switch (tableEntry->entryType)
        {
        case 0: // Processor local APIC
            local_apics[lapicCount] = (LAPIC_ENTRY *)tableEntry;
            lapicCount++;
            break;
        case 1: // External IO-APIC
            ioapic = (IOAPIC_ENTRY *)tableEntry;
            break;
        default:
            break;
        }
    }

    void parse_madt()
    {
        madt = (acpi::MADT *)acpi::findACPITable("APIC");
        lapic = (uint32_t *)madt->LAPIC_addr;

        lapicCount = 0;
        int entryCount = 0;

        void *madtEnd = (void *)((uint8_t *)madt + madt->hdr.length);
        for (uint8_t *ptr = (uint8_t *)(&madt->ptr); ptr < madtEnd; ptr += ptr[1])
        {
            if (ptr[1] == 0 || entryCount >= 12)
                break;
            evaluateMADTEntry((acpi::MADT_ENTRY_HDR *)ptr);
            entryCount++;
        }
    }

}