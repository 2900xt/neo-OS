#include <types.h>
#include <x64/io.h>

constexpr auto INTERRUPT = 0x8E;
constexpr auto EXCEPTION = 0x8F;

struct IDTEntryAMD64
{
    uint16_t offsetLow;
    uint16_t selector;
    uint8_t  interruptStackTable;
    uint8_t  flags;
    uint16_t offsetMid;
    uint32_t offsetHigh;
    uint32_t zero;
}__attribute__((packed));


IDTEntryAMD64 IDT[256];

const char* const exceptions[] = {
    "Divide-by-zero Error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment not Present",
    "Stack-segment Fault",
    "General Protection Fault",
    "Page Fault",
    NULL,
    "x87 FPU exception",
    "Alignment Check",
    "Machine Check",
    "SIMD FPU exception",
    "Virtualization Exception",
    "Control Projection Exception",
    NULL, NULL, NULL, NULL, NULL, NULL,
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception"
};

struct interruptFrame
{
    uint64_t IP;
    uint64_t CS;
    uint64_t flags;
    uint64_t SP;
    uint64_t BP;
}__attribute__ ((packed));

static inline void lidt(void* base, uint16_t size)
{
    struct {
        uint16_t length;
        void*    base;
    } __attribute__((packed)) IDTR = { size, base };

    asm ( "lidt %0" : : "m"(IDTR) );  // let the compiler choose an addressing mode
}

void setIDTEntry(uint8_t vector, void* isr, uint8_t flags){

    if(flags == INTERRUPT){
        vector += 32;
    }

    IDT[vector].flags                     = flags;
    IDT[vector].zero                      = 0x0;
    IDT[vector].selector                  = 40;
    IDT[vector].interruptStackTable       = 0x0;
    IDT[vector].offsetLow                 = (uint16_t)(((uint64_t)isr & 0x000000000000FFFF));
    IDT[vector].offsetMid                 = (uint16_t)(((uint64_t)isr & 0x00000000FFFF0000) >> 16);
    IDT[vector].offsetHigh                = (uint32_t)(((uint64_t)isr & 0xFFFFFFFF00000000) >> 32);
}



const char* exceptionNoError = "IP: \t0x%x\nCS: \t0x%x\nFLAGS: \t0x%x\nSP: \t0x%x\n->\t%s\n";
const char* exceptionError   = "ERROR: 0b%b\nIP: \t0x%x\nCS: \t0x%x\nFLAGS: \t0x%x\nSP: \t0x%x\n->\t%s\n";
//Interrupts

extern "C" {
//Exceptions without error codes

__attribute__ ((interrupt)) void exc0  (interruptFrame* frame)      {klogf(LOG_CRITICAL, exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[0]);  bsp_done();}
__attribute__ ((interrupt)) void exc1  (interruptFrame* frame)      {klogf(LOG_CRITICAL, exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[1]);  bsp_done();}
__attribute__ ((interrupt)) void exc2  (interruptFrame* frame)      {klogf(LOG_CRITICAL, exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[2]);  bsp_done();}
__attribute__ ((interrupt)) void exc3  (interruptFrame* frame)      {klogf(LOG_CRITICAL, exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[3]);  bsp_done();}
__attribute__ ((interrupt)) void exc4  (interruptFrame* frame)      {klogf(LOG_CRITICAL, exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[4]);  bsp_done();}
__attribute__ ((interrupt)) void exc5  (interruptFrame* frame)      {klogf(LOG_CRITICAL, exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[5]);  bsp_done();}
__attribute__ ((interrupt)) void exc6  (interruptFrame* frame)      {klogf(LOG_CRITICAL, exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[6]);  bsp_done();}
__attribute__ ((interrupt)) void exc7  (interruptFrame* frame)      {klogf(LOG_CRITICAL, exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[7]);  bsp_done();}
__attribute__ ((interrupt)) void exc9  (interruptFrame* frame)      {klogf(LOG_CRITICAL, exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[9]);  bsp_done();}
__attribute__ ((interrupt)) void exc16 (interruptFrame* frame)      {klogf(LOG_CRITICAL, exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[16]);  bsp_done();}
__attribute__ ((interrupt)) void exc18 (interruptFrame* frame)      {klogf(LOG_CRITICAL, exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[18]);  bsp_done();}
__attribute__ ((interrupt)) void exc19 (interruptFrame* frame)      {klogf(LOG_CRITICAL, exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[19]);  bsp_done();}
__attribute__ ((interrupt)) void exc20 (interruptFrame* frame)      {klogf(LOG_CRITICAL, exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[20]);  bsp_done();}
__attribute__ ((interrupt)) void exc28 (interruptFrame* frame)      {klogf(LOG_CRITICAL, exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[28]);  bsp_done();}

//Exceptions with error codes

__attribute__ ((interrupt)) void exc8  (interruptFrame* frame, uint64_t error)      {klogf(LOG_CRITICAL, exceptionError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[8]);   bsp_done();}
__attribute__ ((interrupt)) void exc10 (interruptFrame* frame, uint64_t error)      {klogf(LOG_CRITICAL, exceptionError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[10]);   bsp_done();}
__attribute__ ((interrupt)) void exc11 (interruptFrame* frame, uint64_t error)      {klogf(LOG_CRITICAL, exceptionError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[11]);   bsp_done();}
__attribute__ ((interrupt)) void exc12 (interruptFrame* frame, uint64_t error)      {klogf(LOG_CRITICAL, exceptionError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[12]);   bsp_done();}
__attribute__ ((interrupt)) void exc13 (interruptFrame* frame, uint64_t error)      {klogf(LOG_CRITICAL, exceptionError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[13]);   bsp_done();}
__attribute__ ((interrupt)) void exc14 (interruptFrame* frame, uint64_t error)      {klogf(LOG_CRITICAL, exceptionError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[14]);   bsp_done();}
__attribute__ ((interrupt)) void exc17 (interruptFrame* frame, uint64_t error)      {klogf(LOG_CRITICAL, exceptionError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[17]);   bsp_done();}
__attribute__ ((interrupt)) void exc21 (interruptFrame* frame, uint64_t error)      {klogf(LOG_CRITICAL, exceptionError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[21]);   bsp_done();}
__attribute__ ((interrupt)) void exc29 (interruptFrame* frame, uint64_t error)      {klogf(LOG_CRITICAL, exceptionError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[29]);   bsp_done();}
__attribute__ ((interrupt)) void exc30 (interruptFrame* frame, uint64_t error)      {klogf(LOG_CRITICAL, exceptionError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[20]);   bsp_done();}

int64_t countdown = 0;

__attribute__ ((interrupt)) void timer_handler (interruptFrame* frame)
{
    countdown--;
    apicSendEOI();
    return; //iretq
}

__attribute__ ((interrupt)) void spurious_isr (interruptFrame* frame)
{
    return; //iretq
}

}

void sleep(int64_t millis)
{
    countdown = millis;
    while(countdown > 0);
}

void fillIDT(void){

    remapPIC(0x20, 0x28);

    //Exceptions

    setIDTEntry(0,  (void*)exc0,  EXCEPTION);
    setIDTEntry(1,  (void*)exc1,  EXCEPTION);
    setIDTEntry(2,  (void*)exc2,  EXCEPTION);
    setIDTEntry(3,  (void*)exc3,  EXCEPTION);
    setIDTEntry(4,  (void*)exc4,  EXCEPTION);
    setIDTEntry(5,  (void*)exc5,  EXCEPTION);
    setIDTEntry(6,  (void*)exc6,  EXCEPTION);
    setIDTEntry(7,  (void*)exc7,  EXCEPTION);
    setIDTEntry(8,  (void*)exc8,  EXCEPTION);
    setIDTEntry(9,  (void*)exc9,  EXCEPTION);
    setIDTEntry(10, (void*)exc10, EXCEPTION);
    setIDTEntry(11, (void*)exc11, EXCEPTION);
    setIDTEntry(12, (void*)exc12, EXCEPTION);
    setIDTEntry(13, (void*)exc13, EXCEPTION);
    setIDTEntry(14, (void*)exc14, EXCEPTION);
    setIDTEntry(16, (void*)exc16, EXCEPTION);
    setIDTEntry(17, (void*)exc17, EXCEPTION);
    setIDTEntry(18, (void*)exc18, EXCEPTION);
    setIDTEntry(19, (void*)exc19, EXCEPTION);
    setIDTEntry(20, (void*)exc20, EXCEPTION);
    setIDTEntry(21, (void*)exc21, EXCEPTION);
    setIDTEntry(28, (void*)exc28, EXCEPTION);
    setIDTEntry(29, (void*)exc29, EXCEPTION);
    setIDTEntry(30, (void*)exc30, EXCEPTION);

    for(int i = 32; i < 256; i++){
        setIDTEntry(i, (void*)exc1, EXCEPTION);
    }

    setIDTEntry(0, (void*)timer_handler, INTERRUPT);
    setIDTEntry(7, (void*)spurious_isr, INTERRUPT);

    lidt(IDT, sizeof(IDT) - 1);
    
    asm("sti");
    
}
