#include <drivers/acpi/madt.h>
#include <drivers/acpi/sdt.h>

#include <kernel/x64/io.h>


extern acpi::MADT *madt;
extern acpi::LAPIC_ENTRY *local_apics[32];
extern uint64_t lapicCount;
extern uint32_t *lapic;

namespace kernel
{

    static const char *lapic_tag = "LAPIC";

#define APIC_SUPPORTED (1 << 9)
#define APIC_ENABLE (1 << 11)

    // LAPIC registers offset from LAPIC base
    // All APIC registers are 16-byte alligned
    // Must be accessed with DWORD writes

    enum LAPIC_REGS_OFFSET : uint16_t
    {
        // Name                      Offset            initial value
        LAPIC_ID_REG = 0x20,               //??000000h
        LAPIC_VERSION_REG = 0x30,          // 80??0010h
        TASK_PRIORITY_REG = 0x80,          // 00000000h
        ARBITRATION_PRIORITY_REG = 0x90,   // 00000000h
        PROCESSOR_PRIORITY_REG = 0xA0,     // 00000000h
        END_OF_INTERRUPT_REG = 0xB0,       //????????h
        REMOTE_READ_REG = 0xC0,            // 00000000h
        LOGICAL_DESTINATION_REG = 0xD0,    // 00000000h
        DESTINATION_FORMAT_REG = 0xE0,     // FFFFFFFFh
        SPURIOUS_INTERRUPT_REG = 0xF0,     // 000000FFh
        ERROR_STATUS_REG = 0x280,          // 00000000h
        INTERRUPT_CMD_REG_LOW = 0x300,     // 00000000h
        INTERRUPT_CMD_REG_HIGH = 0x310,    // 00000000h
        TIMER_LVT_ENTRY = 0x320,           // 00010000h
        THERMAL_LVT_ENTRY = 0x330,         // 00010000h
        PERFORMACE_CTR_LVT_ENTRY = 0x340,  // 00010000h
        LOCAL_INT0_LVT_ENTRY = 0x350,      // 00010000h
        LOCAL_INT1_LVT_ENTRY = 0x360,      // 00010000h
        ERROR_LVT_ENTRY = 0x370,           // 00010000h
        TIMER_INITIAL_COUNT_REG = 0x380,   // 00000000h
        TIMER_CURRENT_COUNT_REG = 0x390,   // 00000000h
        TIMER_DIVIDE_CONFIG_REG = 0x3E0,   // 00000000h
        EXTENDED_APIC_FEATURE_REG = 0x400, // 00040007h
        EXTENDED_APIC_CONTROL_REG = 0x410, // 00000000h
    };

    enum LVT_FLAGS : uint16_t
    {
        MESSAGE_FIXED = 0b000,
        MESSAGE_SMI = 0b010,
        MESSAGE_NMI = 0b100,
        MESSAGE_EXTERNAL = 0b111,

        DELIVERY_STATUS = (1 << 4),
        REMOTE_IRR = (1 << 6),
        TRIGGER_MODE = (1 << 7),
        INTERRUPT_MASK = (1 << 8),
        TIMER_MODE = (1 << 9)
    };

    static bool apicEnabled = false;

    uint32_t apicReadRegister(uint16_t const reg)
    {
        return *(uint32_t *)((uint64_t)lapic + reg);
    }

    void apicWriteRegister(uint16_t const reg, uint32_t const val)
    {
        *(uint32_t *)((uint64_t)lapic + reg) = val;
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
        // check for APIC

        uint64_t eax, edx;
        cpuid(1, &eax, &edx);
        if (!(edx & APIC_SUPPORTED))
        {
            log::e(lapic_tag, "APIC not supported on this system! (CPUID_ERROR)");
            for (;;)
                ;
        }

        // enable the APIC

        if (!(rdmsr(0x1B) & APIC_ENABLE))
        {
            wrmsr(0x1B, rdmsr(0x1B) | APIC_ENABLE);
        }

        if (!(rdmsr(0x1B) & APIC_ENABLE))
        {
            log::e(lapic_tag, "APIC not suported on this system! (MSR_NOT_PRESENT)");
            for (;;)
                ;
        }

        // parse the MADT for the apic addreses and entries

        acpi::parse_madt();

        apicEnabled = true;
    }

    void initAPIC(const uint8_t apicID)
    {
        if (!apicEnabled)
        {
            enableAPIC();
        }

        if (apicID == 0)
        {
            ioapic_init();
        }

        // Check if apicID matches

        const uint8_t id = apicReadRegister(LAPIC_ID_REG) >> 24;
        if (id != apicID)
        {
            log::e(lapic_tag, "APIC ID not matching: given: %u\t sys:%u", apicID, id);
            return;
        }

        // Set up APIC to a normal state

        apicWriteRegister(DESTINATION_FORMAT_REG, 0xFFFFFFFF);
        apicWriteRegister(LOGICAL_DESTINATION_REG, (apicReadRegister(LOGICAL_DESTINATION_REG) & 0x00FFFFFF) | 0x1);
        apicWriteRegister(TIMER_LVT_ENTRY, INTERRUPT_MASK << 8);
        apicWriteRegister(PERFORMACE_CTR_LVT_ENTRY, MESSAGE_NMI << 8);
        apicWriteRegister(LOCAL_INT0_LVT_ENTRY, INTERRUPT_MASK << 8);
        apicWriteRegister(LOCAL_INT1_LVT_ENTRY, INTERRUPT_MASK << 8);
        apicWriteRegister(TASK_PRIORITY_REG, 0x0);

        if (apicID != 0)
            return;

        // Set up lapic timer

        apicWriteRegister(SPURIOUS_INTERRUPT_REG, 39 | 0x100); // ISR #7
        apicWriteRegister(TIMER_LVT_ENTRY, 32);                // ISR #0
        apicWriteRegister(TIMER_DIVIDE_CONFIG_REG, 0x3);       // Divide by 16

        // Use the PIT to get the amount of ticks for a second

        outb(0x61, (inb(0x61) & 0x0FD) | 0x1);
        outb(0x43, 0b10110010);
        outb(0x42, 0x9B);
        IO_WAIT();
        outb(0x42, 0x2e);
        outb(0x61, (inb(0x61) & 0x0FE) | 0x1);

        apicWriteRegister(TIMER_INITIAL_COUNT_REG, -1);

        while ((inb(0x61) & 0x20))
            ;

        uint32_t cpuFreq = apicReadRegister(TIMER_INITIAL_COUNT_REG) - apicReadRegister(TIMER_CURRENT_COUNT_REG);

        apicWriteRegister(TIMER_LVT_ENTRY, INTERRUPT_MASK << 8);

        // Calculate CPU frequency

        cpuFreq *= 16 * 100;
        cpuFreq /= 16;
        // cpuFreq /= 1000;

        log::d(lapic_tag, "CPU frequency: %d MHz", (cpuFreq / 1000));

        apicWriteRegister(TIMER_INITIAL_COUNT_REG, cpuFreq);
        apicWriteRegister(TIMER_LVT_ENTRY, 32 | 0x20000);
        apicWriteRegister(TIMER_DIVIDE_CONFIG_REG, 0x3);
        log::d(lapic_tag, "APIC timer initialized");
    }

}