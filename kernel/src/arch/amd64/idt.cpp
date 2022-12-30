#include <stdout.h>
#include <arch/amd64/io.h>

enum IDT_FLAGS
{
    PRESENT         = (1 << 7),
    USER_ACCESSIBLE = (3 << 5),
    INTERRUPT       = 0xE,
    EXCEPTION       = 0xF,
};

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
bool isrs[16];

const char* exceptions[] = {
    "Divide-by-zero Error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device not Avalible",
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
};

static inline void lidt(void* base, uint16_t size = 128 * 256 - 1)
{
    struct {
        uint16_t length;
        void*    base;
    } __attribute__((packed)) IDTR = { size, base };

    asm ( "lidt %0" : : "m"(IDTR) );  // let the compiler choose an addressing mode
}

void setIDTEntry(uint8_t vector, uint64_t isr, uint8_t flags){
    IDT[vector].flags                     = flags;
    IDT[vector].zero                      = 0;
    IDT[vector].selector                  = 0x8;
    IDT[vector].interruptStackTable       = 0;
    IDT[vector].offsetLow                 = (uint16_t)((uint64_t)(isr & 0xFFFF));
    IDT[vector].offsetMid                 = (uint16_t)((uint64_t)(isr & 0xFFFF0000) >> 16);
    IDT[vector].offsetHigh                = (uint32_t)((uint64_t)(isr & 0xFFFFFFFF00000000) >> 32);
}

//Interrupts

__attribute__ ((interrupt))  void isr0  (interruptFrame* frame)   {isrs[0] = true;  sendEOI(0); }
__attribute__ ((interrupt))  void isr1  (interruptFrame* frame)   {isrs[1] = true;  sendEOI(1); }
__attribute__ ((interrupt))  void isr2  (interruptFrame* frame)   {isrs[2] = true;  sendEOI(2); }
__attribute__ ((interrupt))  void isr3  (interruptFrame* frame)   {isrs[3] = true;  sendEOI(3); }
__attribute__ ((interrupt))  void isr4  (interruptFrame* frame)   {isrs[4] = true;  sendEOI(4); }
__attribute__ ((interrupt))  void isr5  (interruptFrame* frame)   {isrs[5] = true;  sendEOI(5); }
__attribute__ ((interrupt))  void isr6  (interruptFrame* frame)   {isrs[6] = true;  sendEOI(6); }
__attribute__ ((interrupt))  void isr7  (interruptFrame* frame)   {isrs[7] = true;  sendEOI(7); }
__attribute__ ((interrupt))  void isr8  (interruptFrame* frame)   {isrs[8] = true;  sendEOI(8); }
__attribute__ ((interrupt))  void isr9  (interruptFrame* frame)   {isrs[9] = true;  sendEOI(9); }
__attribute__ ((interrupt))  void isr10 (interruptFrame* frame)   {isrs[10] = true; sendEOI(10);}
__attribute__ ((interrupt))  void isr11 (interruptFrame* frame)   {isrs[11] = true; sendEOI(11);}
__attribute__ ((interrupt))  void isr12 (interruptFrame* frame)   {isrs[12] = true; sendEOI(12);}
__attribute__ ((interrupt))  void isr13 (interruptFrame* frame)   {isrs[13] = true; sendEOI(13);}
__attribute__ ((interrupt))  void isr14 (interruptFrame* frame)   {isrs[14] = true; sendEOI(14);}
__attribute__ ((interrupt))  void isr15 (interruptFrame* frame)   {isrs[15] = true; sendEOI(15);}

//Exceptions without error codes

const char* exceptionNoError = "%s->\nIP: %x\nCS: %x\nFLAGS: %x\nSP: %x\n\n\nDone!";

__attribute__ ((interrupt)) void exc0  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, exceptions[0],  frame->IP, frame->CS, frame->flags, frame->SP);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc1  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, exceptions[1],  frame->IP, frame->CS, frame->flags, frame->SP);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc2  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, exceptions[2],  frame->IP, frame->CS, frame->flags, frame->SP);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc3  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, exceptions[3],  frame->IP, frame->CS, frame->flags, frame->SP);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc4  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, exceptions[4],  frame->IP, frame->CS, frame->flags, frame->SP);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc5  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, exceptions[5],  frame->IP, frame->CS, frame->flags, frame->SP);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc6  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, exceptions[6],  frame->IP, frame->CS, frame->flags, frame->SP);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc7  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, exceptions[7],  frame->IP, frame->CS, frame->flags, frame->SP);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc9  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, exceptions[9],  frame->IP, frame->CS, frame->flags, frame->SP);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc16 (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, exceptions[16], frame->IP, frame->CS, frame->flags, frame->SP);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc18 (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, exceptions[18], frame->IP, frame->CS, frame->flags, frame->SP);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc19 (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, exceptions[19], frame->IP, frame->CS, frame->flags, frame->SP);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc20 (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, exceptions[20], frame->IP, frame->CS, frame->flags, frame->SP);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc28 (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, exceptions[28], frame->IP, frame->CS, frame->flags, frame->SP);  neoOS_STD::done();}

//Exceptions with error codes

const char* exceptionError = "%s->\nCode: %x\n\nIP: %x\nCS: %x\nFLAGS: %x\nSP: %x\n\n\nDone!";

__attribute__ ((interrupt)) void exc8  (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, exceptions[8],  error, frame->IP, frame->CS, frame->flags, frame->SP);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc10 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, exceptions[10], error, frame->IP, frame->CS, frame->flags, frame->SP);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc11 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, exceptions[11], error, frame->IP, frame->CS, frame->flags, frame->SP);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc12 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, exceptions[12], error, frame->IP, frame->CS, frame->flags, frame->SP);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc13 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, exceptions[13], error, frame->IP, frame->CS, frame->flags, frame->SP);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc14 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, exceptions[14], error, frame->IP, frame->CS, frame->flags, frame->SP);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc17 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, exceptions[17], error, frame->IP, frame->CS, frame->flags, frame->SP);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc21 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, exceptions[21], error, frame->IP, frame->CS, frame->flags, frame->SP);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc29 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, exceptions[29], error, frame->IP, frame->CS, frame->flags, frame->SP);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc30 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, exceptions[20], error, frame->IP, frame->CS, frame->flags, frame->SP);   neoOS_STD::done();}
        
void fillIDT(void){

    remapPIC(32, 40);

    //Interrupts

    setIDTEntry(32 + 0,  (uint64_t)isr0,  PRESENT | INTERRUPT);
    setIDTEntry(32 + 1,  (uint64_t)isr1,  PRESENT | INTERRUPT);
    setIDTEntry(32 + 2,  (uint64_t)isr2,  PRESENT | INTERRUPT);
    setIDTEntry(32 + 3,  (uint64_t)isr3,  PRESENT | INTERRUPT);
    setIDTEntry(32 + 4,  (uint64_t)isr4,  PRESENT | INTERRUPT);
    setIDTEntry(32 + 5,  (uint64_t)isr5,  PRESENT | INTERRUPT);
    setIDTEntry(32 + 6,  (uint64_t)isr6,  PRESENT | INTERRUPT);
    setIDTEntry(32 + 7,  (uint64_t)isr7,  PRESENT | INTERRUPT);
    setIDTEntry(32 + 8,  (uint64_t)isr8,  PRESENT | INTERRUPT);
    setIDTEntry(32 + 9,  (uint64_t)isr9,  PRESENT | INTERRUPT);
    setIDTEntry(32 + 10, (uint64_t)isr10, PRESENT | INTERRUPT);
    setIDTEntry(32 + 11, (uint64_t)isr11, PRESENT | INTERRUPT);
    setIDTEntry(32 + 12, (uint64_t)isr12, PRESENT | INTERRUPT);
    setIDTEntry(32 + 13, (uint64_t)isr13, PRESENT | INTERRUPT);
    setIDTEntry(32 + 14, (uint64_t)isr14, PRESENT | INTERRUPT);
    setIDTEntry(32 + 15, (uint64_t)isr15, PRESENT | INTERRUPT);

    //Exceptions


    setIDTEntry(0,  (uint64_t)exc0,  PRESENT | EXCEPTION);
    setIDTEntry(1,  (uint64_t)exc1,  PRESENT | EXCEPTION);
    setIDTEntry(2,  (uint64_t)exc2,  PRESENT | EXCEPTION);
    setIDTEntry(3,  (uint64_t)exc3,  PRESENT | EXCEPTION);
    setIDTEntry(4,  (uint64_t)exc4,  PRESENT | EXCEPTION);
    setIDTEntry(5,  (uint64_t)exc5,  PRESENT | EXCEPTION);
    setIDTEntry(6,  (uint64_t)exc6,  PRESENT | EXCEPTION);
    setIDTEntry(7,  (uint64_t)exc7,  PRESENT | EXCEPTION);
    setIDTEntry(8,  (uint64_t)exc8,  PRESENT | EXCEPTION);
    setIDTEntry(9,  (uint64_t)exc9,  PRESENT | EXCEPTION);
    setIDTEntry(10, (uint64_t)exc10, PRESENT | EXCEPTION);
    setIDTEntry(11, (uint64_t)exc11, PRESENT | EXCEPTION);
    setIDTEntry(12, (uint64_t)exc12, PRESENT | EXCEPTION);
    setIDTEntry(13, (uint64_t)exc13, PRESENT | EXCEPTION);
    setIDTEntry(14, (uint64_t)exc14, PRESENT | EXCEPTION);
    setIDTEntry(16, (uint64_t)exc16, PRESENT | EXCEPTION);
    setIDTEntry(17, (uint64_t)exc17, PRESENT | EXCEPTION);
    setIDTEntry(18, (uint64_t)exc18, PRESENT | EXCEPTION);
    setIDTEntry(19, (uint64_t)exc19, PRESENT | EXCEPTION);
    setIDTEntry(20, (uint64_t)exc20, PRESENT | EXCEPTION);
    setIDTEntry(21, (uint64_t)exc21, PRESENT | EXCEPTION);
    setIDTEntry(28, (uint64_t)exc28, PRESENT | EXCEPTION);
    setIDTEntry(29, (uint64_t)exc29, PRESENT | EXCEPTION);
    setIDTEntry(30, (uint64_t)exc30, PRESENT | EXCEPTION);

    lidt(IDT);

    asm("sti");

}




#define PIC1		0x20		/* IO base address for master PIC */
#define PIC2		0xA0		/* IO base address for slave PIC */
#define PIC1_COMMAND	PIC1
#define PIC1_DATA	(PIC1+1)
#define PIC2_COMMAND	PIC2
#define PIC2_DATA	(PIC2+1)

#define ICW1_ICW4	0x01		/* ICW4 (not) needed */
#define ICW1_SINGLE	0x02		/* Single (cascade) mode */
#define ICW1_INTERVAL4	0x04		/* Call address interval 4 (8) */
#define ICW1_LEVEL	0x08		/* Level triggered (edge) mode */
#define ICW1_INIT	0x10		/* Initialization - required! */

#define ICW4_8086	0x01		/* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO	0x02		/* Auto (normal) EOI */
#define ICW4_BUF_SLAVE	0x08		/* Buffered mode/slave */
#define ICW4_BUF_MASTER	0x0C		/* Buffered mode/master */
#define ICW4_SFNM	0x10		/* Special fully nested (not) */


//PIC Code from OSDEV wiki

#define PIC_EOI		0x20		/* End-of-interrupt command code */

void sendEOI(unsigned char irq)
{
	if(irq >= 8)
		outb(PIC2_COMMAND,PIC_EOI);

	outb(PIC1_COMMAND,PIC_EOI);
}

void remapPIC(int offset1, int offset2)
{
	unsigned char a1, a2;

	a1 = inb(PIC1_DATA);                        // save masks
	a2 = inb(PIC2_DATA);

	outb(PIC1_COMMAND, ICW1_INIT | ICW1_ICW4);  // starts the initialization sequence (in cascade mode)
	 
	outb(PIC2_COMMAND, ICW1_INIT | ICW1_ICW4);
	 
	outb(PIC1_DATA, offset1);                 // ICW2: Master PIC vector offset
	 
	outb(PIC2_DATA, offset2);                 // ICW2: Slave PIC vector offset
	 
	outb(PIC1_DATA, 4);                       // ICW3: tell Master PIC that there is a slave PIC at IRQ2 (0000 0100)
	 
	outb(PIC2_DATA, 2);                       // ICW3: tell Slave PIC its cascade identity (0000 0010)
	 

	outb(PIC1_DATA, ICW4_8086);
	 
	outb(PIC2_DATA, ICW4_8086);
	 

	outb(PIC1_DATA, a1);   // restore saved masks.
	outb(PIC2_DATA, a2);
}


