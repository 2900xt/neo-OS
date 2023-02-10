#include <types.h>

#ifndef AMD64_IO_H
#define ADM64_IO_H


#define APIC_SUPPORTED  (1 << 9)
#define APIC_ENABLE     (1 << 11)

//LAPIC registers offset from LAPIC base
//All APIC registers are 16-byte alligned
//Must be accessed with DWORD writes

enum LAPIC_REGS_OFFSET : uint16_t
{
    //Name                      Offset            initial value
    LAPIC_ID_REG                = 0x20,         //??000000h
    LAPIC_VERSION_REG           = 0x30,         //80??0010h
    TASK_PRIORITY_REG           = 0x80,         //00000000h
    ARBITRATION_PRIORITY_REG    = 0x90,         //00000000h
    PROCESSOR_PRIORITY_REG      = 0xA0,         //00000000h
    END_OF_INTERRUPT_REG        = 0xB0,         //????????h
    REMOTE_READ_REG             = 0xC0,         //00000000h
    LOGICAL_DESTINATION_REG     = 0xD0,         //00000000h
    DESTINATION_FORMAT_REG      = 0xE0,         //FFFFFFFFh
    SPURIOUS_INTERRUPT_REG      = 0xF0,         //000000FFh
    ERROR_STATUS_REG            = 0x280,        //00000000h
    INTERRUPT_CMD_REG_LOW       = 0x300,        //00000000h
    INTERRUPT_CMD_REG_HIGH      = 0x310,        //00000000h
    TIMER_LVT_ENTRY             = 0x320,        //00010000h
    THERMAL_LVT_ENTRY           = 0x330,        //00010000h
    PERFORMACE_CTR_LVT_ENTRY    = 0x340,        //00010000h
    LOCAL_INT0_LVT_ENTRY        = 0x350,        //00010000h
    LOCAL_INT1_LVT_ENTRY        = 0x360,        //00010000h
    ERROR_LVT_ENTRY             = 0x370,        //00010000h
    TIMER_INITIAL_COUNT_REG     = 0x380,        //00000000h
    TIMER_CURRENT_COUNT_REG     = 0x390,        //00000000h
    TIMER_DIVIDE_CONFIG_REG     = 0x3E0,        //00000000h
    EXTENDED_APIC_FEATURE_REG   = 0x400,        //00040007h
    EXTENDED_APIC_CONTROL_REG   = 0x410,        //00000000h
};

enum LVT_FLAGS : uint16_t
{
    MESSAGE_FIXED              = 0b000,
    MESSAGE_SMI                = 0b010,
    MESSAGE_NMI                = 0b100,
    MESSAGE_EXTERNAL           = 0b111,
    
    DELIVERY_STATUS            = (1 << 4),
    REMOTE_IRR                 = (1 << 6),
    TRIGGER_MODE               = (1 << 7),
    INTERRUPT_MASK             = (1 << 8),
    TIMER_MODE                 = (1 << 9)
};


static inline void outb(uint16_t port, uint8_t val){
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port){
    uint8_t val;
    asm volatile("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void outl(uint16_t port, uint32_t val)
{
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint32_t inl(uint16_t port){
    uint32_t val;
    asm volatile("inl %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline void cpuid(int code, uint64_t* a, uint64_t* d)
{
    asm volatile ( "cpuid" : "=a"(*a), "=d"(*d) : "0"(code) : "rbx", "rcx" );
}

static inline uint64_t rdtsc(void)
{
    uint32_t low, high;
    asm volatile("rdtsc":"=a"(low),"=d"(high));
    return ((uint64_t)high << 32) | low;
}

static inline void wrmsr(uint64_t msr, uint64_t value)
{
	uint32_t low = value & 0xFFFFFFFF;
	uint32_t high = value >> 32;
	asm volatile (
		"wrmsr"
		:
		: "c"(msr), "a"(low), "d"(high)
	);
}

static inline uint64_t rdmsr(uint64_t msr)
{
	uint32_t low, high;
	asm volatile (
		"rdmsr"
		: "=a"(low), "=d"(high)
		: "c"(msr)
	);
	return ((uint64_t)high << 32) | low;
}

uint64_t readCR3(void);

void remapPIC(int offset1, int offset2);
void fillIDT(void);
extern "C" void enableSSE(void);
void apicSendEOI(void);
void sleep(int64_t millis);

#define IO_WAIT() outb(0x80, 0)

#endif // !AMD64_IO_H