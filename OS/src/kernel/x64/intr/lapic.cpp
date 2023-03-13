#include "drivers/acpi/madt.h"
#include <drivers/acpi/sdt.h>
#include <kernel/x64/intr/apic.h>
#include <kernel/x64/io.h>
#include <stdlib/stdlib.h>

extern ACPI::MADT*              madt;
extern kernel::LAPIC_ENTRY*     local_apics[32];
extern uint64_t                 lapicCount;
extern uint32_t*                lapic;

namespace kernel 
{
static bool apicEnabled = false;

uint32_t apicReadRegister(uint16_t const reg)
{
    return *(uint32_t*)((uint64_t)lapic + reg);
}

void apicWriteRegister(uint16_t const reg, uint32_t const val)
{
    *(uint32_t*)((uint64_t)lapic + reg) = val;
}

void apicSendEOI(void)
{
    apicWriteRegister(END_OF_INTERRUPT_REG, 0);
}

void setLVTEntry(uint16_t const entry, uint8_t const vector, uint16_t const flags)
{
    const uint32_t data = vector | (flags << 8);
    apicWriteRegister(entry, data);
}

void enableAPIC(void)
{
    //check for APIC

    uint64_t eax, edx;
    cpuid(1, &eax, &edx);
    if(!(edx & APIC_SUPPORTED))
    {
        std::klogf("APIC not supported on this system! (CPUID_ERROR)\n");
        for(;;);
    }

    //enable the APIC

    if(!(rdmsr(0x1B) & APIC_ENABLE))
    {
        wrmsr(0x1B, rdmsr(0x1B) | APIC_ENABLE);
    }

    if(!(rdmsr(0x1B) & APIC_ENABLE))
    {
        std::klogf("APIC not suported on this system! (MSR_NOT_PRESENT)\n");
        for(;;);
    }

    //parse the MADT for the apic addreses and entries

    ACPI::parse_madt();

    apicEnabled = true;

}


void initAPIC(const uint8_t apicID)
{
    if(!apicEnabled) {
        enableAPIC();
    }

    if(apicID == 0)
    {
        ioapic_init();
    }

    //Check if apicID matches

    const uint8_t id = apicReadRegister(LAPIC_ID_REG) >> 24;
    if(id != apicID)
    {
        std::klogf("APIC ID not matching: given: %d\t sys:%d\n", apicID, id);
        return;
    }

    //Set up APIC to a normal state

    apicWriteRegister(DESTINATION_FORMAT_REG, 0xFFFFFFFF);
    apicWriteRegister(LOGICAL_DESTINATION_REG, (apicReadRegister(LOGICAL_DESTINATION_REG) & 0x00FFFFFF) | 0x1);
    apicWriteRegister(TIMER_LVT_ENTRY, INTERRUPT_MASK << 8);
    apicWriteRegister(PERFORMACE_CTR_LVT_ENTRY, MESSAGE_NMI << 8);
    apicWriteRegister(LOCAL_INT0_LVT_ENTRY, INTERRUPT_MASK << 8);
    apicWriteRegister(LOCAL_INT1_LVT_ENTRY, INTERRUPT_MASK << 8);
    apicWriteRegister(TASK_PRIORITY_REG, 0x0);

    if(apicID != 0) return;

    //Set up lapic timer

    apicWriteRegister(SPURIOUS_INTERRUPT_REG, 39 | 0x100);      //ISR #7
    apicWriteRegister(TIMER_LVT_ENTRY, 32);                     //ISR #0
    apicWriteRegister(TIMER_DIVIDE_CONFIG_REG, 0x3);            //Divide by 16

    //Use the PIT to get the amount of ticks for a second

    outb(0x61, (inb(0x61) & 0x0FD) | 0x1);
    outb(0x43, 0b10110010);
    outb(0x42, 0x9B);
    IO_WAIT();
    outb(0x42, 0x2e);
    outb(0x61, (inb(0x61) & 0x0FE) | 0x1);

    apicWriteRegister(TIMER_INITIAL_COUNT_REG, -1);

    while((inb(0x61) & 0x20));

    uint32_t cpuFreq = apicReadRegister(TIMER_INITIAL_COUNT_REG) - apicReadRegister(TIMER_CURRENT_COUNT_REG);

    apicWriteRegister(TIMER_LVT_ENTRY, INTERRUPT_MASK << 8);

    //Calculate CPU frequency

    cpuFreq *= 16 * 100;

    std::klogf("CPU freq: %d mhz\n", (uint64_t)cpuFreq / 1000000);

    cpuFreq /= 16;
    cpuFreq /= 1000;

    apicWriteRegister(TIMER_INITIAL_COUNT_REG, cpuFreq);
    apicWriteRegister(TIMER_LVT_ENTRY, 32 | 0x20000);
    apicWriteRegister(TIMER_DIVIDE_CONFIG_REG, 0x3);
}

}