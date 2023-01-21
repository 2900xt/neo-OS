#include <acpi/tables.h>
#include <stdout.h>
#include <mem.h>
#include <arch/amd64/io.h>

ACPI_MADT*      madt;
LAPIC_ENTRY*    local_apics[32];
uint64_t        lapicCount;
IOAPIC_ENTRY*   ioapic;
uint32_t*       lapic;

bool            apicEnabled = false;


uint32_t apicReadRegister(uint16_t reg)
{
    return *(uint32_t*)((uint64_t)lapic + reg);
}

void apicWriteRegister(uint16_t reg, uint32_t val)
{
    *(uint32_t*)((uint64_t)lapic + reg) = val;
}

void apicSendEOI(void)
{
    apicWriteRegister(END_OF_INTERRUPT_REG, 0);
}

void setLVTEntry(uint16_t entry, uint8_t vector, uint16_t flags)
{
    uint32_t data = vector | (flags << 8);
    apicWriteRegister(entry, data);
}


void evaluateMADTEntry(ACPI_MADT_ENTRY_HDR* tableEntry)
{
    switch (tableEntry->entryType)
    {
        case 0: //Processor local APIC
            local_apics[lapicCount] = (LAPIC_ENTRY*)tableEntry;

            klogf(LOG_DEBUG, 
            "LAPIC found -> PID: %d APIC ID: %d\n",
            local_apics[lapicCount]->processor_id,
            local_apics[lapicCount]->apic_id
            );

            lapicCount++;
            break;
        case 1:
            ioapic = (IOAPIC_ENTRY*)tableEntry;

            klogf(LOG_DEBUG,
            "IOAPIC found -> addr 0x%x ID: %d\n",
            ioapic->ioapic_base,
            ioapic->ioapic_id
            );

            break;
        case 5:
            klogf(0,"Sorry, 64-bit APIC unsuported!\n");
            bsp_done();
        default:
            break;
    }
}

void enableAPIC(void)
{
    //check for APIC

    uint64_t eax, edx;
    cpuid(1, &eax, &edx);
    if(!(edx & APIC_SUPPORTED))
    {
        klogf(LOG_CRITICAL, "APIC not supported on this system! (CPUID_ERROR)\n");
        bsp_done();
    }

    //enable the APIC

    if(!(rdmsr(0x1B) & APIC_ENABLE))
    {
        wrmsr(0x1B, rdmsr(0x1B) | APIC_ENABLE);
    }

    if(!(rdmsr(0x1B) & APIC_ENABLE))
    {
        klogf(LOG_CRITICAL, "APIC not suported on this system! (MSR_NOT_PRESENT)\n");
        bsp_done();
    }

    //parse the MADT for the apic addreses and entries

    madt = (ACPI_MADT*)findACPITable("APIC");
    lapic = (uint32_t*)madt->LAPIC_addr;

    lapicCount = 0;

    void* madtEnd = (void*)((uint8_t*)madt + madt->hdr.length);
    for(register uint8_t* ptr = (uint8_t*)(&madt->ptr); ptr < madtEnd; ptr += ptr[1])
    {
        if(ptr[1] == 0)
            break;
        evaluateMADTEntry((ACPI_MADT_ENTRY_HDR*)ptr);
    }

    apicEnabled = true;

}


void initAPIC(uint8_t apicID)
{
    //Enable APIC

    if(!apicEnabled) {
        enableAPIC();
    }

    //Check if apicID matches

    uint8_t id = apicReadRegister(LAPIC_ID_REG) >> 24;
    if(id != apicID)
    {
        klogf(0, "APIC ID not matching: given: %d\t sys:%d\n", apicID, id);
        return;
    }
    

    //Set up APIC to a normal state

    apicWriteRegister(DESTINATION_FORMAT_REG, 0xFFFFFFFF);
    apicWriteRegister(LOGICAL_DESTINATION_REG, (apicReadRegister(LOGICAL_DESTINATION_REG) & 0x00FFFFFF) | 1);
    apicWriteRegister(TIMER_LVT_ENTRY, INTERRUPT_MASK);

    //Enable lapic interrupts

    apicWriteRegister(SPURIOUS_INTERRUPT_REG, apicReadRegister(SPURIOUS_INTERRUPT_REG) | 0x1FF);

    //Set up lapic timer



}

