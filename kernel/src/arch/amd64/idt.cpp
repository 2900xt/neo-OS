#include <stdout.h>
#include <arch/amd64/io.h>

#define INTERRUPT 0x8E
#define EXCEPTION 0x8F

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

typedef struct
{
    uint64_t IP;
    uint64_t CS;
    uint64_t flags;
    uint64_t SP;
    uint64_t BP;
} interruptFrame;

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
const char* exceptionError   = "ERROR: 0x%x\nIP: \t0x%x\nCS: \t0x%x\nFLAGS: \t0x%x\nSP: \t0x%x\n->\t%s\n";
//Interrupts

extern "C"{

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

__attribute__ ((interrupt)) void exc0  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[0]);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc1  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[1]);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc2  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[2]);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc3  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[3]);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc4  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[4]);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc5  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[5]);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc6  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[6]);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc7  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[7]);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc9  (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[9]);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc16 (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[16]);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc18 (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[18]);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc19 (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[19]);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc20 (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[20]);  neoOS_STD::done();}
__attribute__ ((interrupt)) void exc28 (interruptFrame* frame)      {neoOS_STD::printf(exceptionNoError, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[28]);  neoOS_STD::done();}

//Exceptions with error codes

__attribute__ ((interrupt)) void exc8  (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[8]);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc10 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[10]);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc11 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[11]);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc12 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[12]);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc13 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[13]);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc14 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[14]);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc17 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[17]);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc21 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[21]);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc29 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[29]);   neoOS_STD::done();}
__attribute__ ((interrupt)) void exc30 (interruptFrame* frame, uint64_t error)      {neoOS_STD::printf(exceptionNoError, error, frame->IP, frame->CS, frame->flags, frame->SP, exceptions[20]);   neoOS_STD::done();}
    
}

void fillIDT(void){

    remapPIC(32, 40);

    //Interrupts

    setIDTEntry(0,  (void*)isr0,  INTERRUPT);
    setIDTEntry(1,  (void*)isr1,  INTERRUPT);
    setIDTEntry(2,  (void*)isr2,  INTERRUPT);
    setIDTEntry(3,  (void*)isr3,  INTERRUPT);
    setIDTEntry(4,  (void*)isr4,  INTERRUPT);
    setIDTEntry(5,  (void*)isr5,  INTERRUPT);
    setIDTEntry(6,  (void*)isr6,  INTERRUPT);
    setIDTEntry(7,  (void*)isr7,  INTERRUPT);
    setIDTEntry(8,  (void*)isr8,  INTERRUPT);
    setIDTEntry(9,  (void*)isr9,  INTERRUPT);
    setIDTEntry(10, (void*)isr10, INTERRUPT);
    setIDTEntry(11, (void*)isr11, INTERRUPT);
    setIDTEntry(12, (void*)isr12, INTERRUPT);
    setIDTEntry(13, (void*)isr13, INTERRUPT);
    setIDTEntry(14, (void*)isr14, INTERRUPT);
    setIDTEntry(15, (void*)isr15, INTERRUPT);

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

    for(int i = 48; i < 256; i++){
        setIDTEntry(i, (void*)exc1, EXCEPTION);
    }

    lidt(IDT, sizeof(IDT) - 1);

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


