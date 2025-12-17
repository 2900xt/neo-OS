#include "drivers/vga/fonts.h"
#include "drivers/vga/vga.h"
#include "kernel/shell/shell.h"
#include "stdlib/timer.h"
#include <types.h>

#include <kernel/io/log.h>
#include <kernel/x64/io.h>
#include <kernel/x64/intr/idt.h>
#include <kernel/x64/intr/apic.h>


uint64_t millis_since_boot = 0;
#define stop() for (;;)

namespace kernel
{
    constexpr uint8_t INTERRUPT = 0x8E;
    constexpr uint8_t EXCEPTION = 0x8F;

    struct IDTEntryAMD64
    {
        uint16_t offsetLow;
        uint16_t selector;
        uint8_t interruptStackTable;
        uint8_t flags;
        uint16_t offsetMid;
        uint32_t offsetHigh;
        uint32_t zero;
    } __attribute__((packed));

    IDTEntryAMD64 IDT[256];

    const char *const exceptions[] = {
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
        "Security Exception"};

    static inline void lidt(void *base, uint16_t size)
    {
        struct
        {
            uint16_t length;
            void *base;
        } __attribute__((packed)) IDTR = {size, base};

        asm("lidt %0" : : "m"(IDTR)); // let the compiler choose an addressing mode
    }

    void setIDTEntry(uint8_t vector, void *isr, uint8_t flags)
    {
        IDT[vector].flags = flags;
        IDT[vector].zero = 0x0;
        IDT[vector].selector = 40;
        IDT[vector].interruptStackTable = 0x0;
        IDT[vector].offsetLow = (uint16_t)(((uint64_t)isr & 0x000000000000FFFF));
        IDT[vector].offsetMid = (uint16_t)(((uint64_t)isr & 0x00000000FFFF0000) >> 16);
        IDT[vector].offsetHigh = (uint32_t)(((uint64_t)isr & 0xFFFFFFFF00000000) >> 32);
    }

    void register_isr(uint8_t vector, __attribute__((interrupt)) void (*isr)(interruptFrame *))
    {
        setIDTEntry(vector, (void *)isr, INTERRUPT);
    }

    const char *exceptionNoError = "IP: \t0x%x\nCS: \t0x%x\nFLAGS: \t0x%x\nSP: \t0x%x\n->\t%s\n";
    const char *exceptionError = "ERROR: 0b%b\nIP: \t0x%x\nCS: \t0x%x\nFLAGS: \t0x%x\nSP: \t0x%x\n->\t%s\n";
    // Interrupts

    const char *intr_tag = "Interrupt";
    const char *except_tag = "Exception";

    // Helper function to convert uint64_t to hex string
    void uint64_to_hex(uint64_t value, char *buffer)
    {
        const char hex_chars[] = "0123456789ABCDEF";
        buffer[0] = '0';
        buffer[1] = 'x';
        for (int i = 15; i >= 0; i--)
        {
            buffer[2 + (15 - i)] = hex_chars[(value >> (i * 4)) & 0xF];
        }
        buffer[18] = '\0';
    }

    extern int terminal_x , terminal_y;

    void kernel_panic(kernel::interruptFrame *frame, const char *message, const char *fmt, uint64_t error, bool has_error)
    {

        
        terminal_x = 0, terminal_y =0;
        // Clear screen to blue (BSOD style)
        vga::fillRect(0, 0, vga::Color(0, 0, 255), vga::fbuf_info->width, vga::fbuf_info->height);
        vga::set_foreground(vga::Color(255, 255, 255));
        vga::set_background(vga::Color(0, 0, 255));

        display_fetch();
        
        // Print to screen using printf
        printf("=== KERNEL PANIC ===\n\n");
        printf("Exception: %s\n", message);
        
        if (has_error)
        {
            printf("Error Code: 0x%x\n\n", error);
        }
        else
        {
            printf("\n");
        }
        
        printf("--- Registers ---\n");
        printf("RIP:    0x%x\n", frame->IP);
        printf("RSP:    0x%x\n", frame->SP);
        printf("RBP:    0x%x\n", frame->BP);
        printf("CS:     0x%x\n", frame->CS);
        printf("RFLAGS: 0x%x\n\n", frame->flags);
        
        printf("--- Stack Trace ---\n");
        uint64_t *stack = (uint64_t *)frame->SP;
        for (int i = 0; i < 8; i++)
        {
            printf("  [%d] 0x%x: 0x%x\n", i, (uint64_t)&stack[i], stack[i]);
        }
        
        vga::repaintScreen();
        
        // Log to serial
        if (has_error)
        {
            log.e(except_tag, fmt, error, frame->IP, frame->CS, frame->flags, frame->SP, message);
        }
        else
        {
            log.e(except_tag, fmt, frame->IP, frame->CS, frame->flags, frame->SP, message);
        }


        
        stop();
    }

    void kernel_panic(kernel::interruptFrame *frame, const char *message, const char *fmt)
    {
        kernel_panic(frame, message, fmt, 0, false);
    }


    extern "C"
    {
        // Exceptions without error codes

        __attribute__((interrupt)) void exc0(interruptFrame *frame)
        {
            kernel_panic(frame, exceptions[0], exceptionNoError);
            stop();
        }
        __attribute__((interrupt)) void exc1(interruptFrame *frame)
        {
            kernel_panic(frame, exceptions[1], exceptionNoError);
            stop();
        }
        __attribute__((interrupt)) void exc2(interruptFrame *frame)
        {
            kernel_panic(frame, exceptions[2], exceptionNoError);
            stop();
        }
        __attribute__((interrupt)) void exc3(interruptFrame *frame)
        {
            kernel_panic(frame, exceptions[3], exceptionNoError);
            stop();
        }
        __attribute__((interrupt)) void exc4(interruptFrame *frame)
        {
            kernel_panic(frame, exceptions[4], exceptionNoError);
            stop();
        }
        __attribute__((interrupt)) void exc5(interruptFrame *frame)
        {
            kernel_panic(frame, exceptions[5], exceptionNoError);
            stop();
        }
        __attribute__((interrupt)) void exc6(interruptFrame *frame)
        {
            kernel_panic(frame, exceptions[6], exceptionNoError);
            stop();
        }
        __attribute__((interrupt)) void exc7(interruptFrame *frame)
        {
            kernel_panic(frame, exceptions[7], exceptionNoError);
            stop();
        }
        __attribute__((interrupt)) void exc9(interruptFrame *frame)
        {
            kernel_panic(frame, exceptions[9], exceptionNoError);
            stop();
        }
        __attribute__((interrupt)) void exc16(interruptFrame *frame)
        {
            kernel_panic(frame, exceptions[16], exceptionNoError);
            stop();
        }
        __attribute__((interrupt)) void exc18(interruptFrame *frame)
        {
            kernel_panic(frame, exceptions[18], exceptionNoError);
            stop();
        }
        __attribute__((interrupt)) void exc19(interruptFrame *frame)
        {
            kernel_panic(frame, exceptions[19], exceptionNoError);
            stop();
        }
        __attribute__((interrupt)) void exc20(interruptFrame *frame)
        {
            kernel_panic(frame, exceptions[20], exceptionNoError);
            stop();
        }
        __attribute__((interrupt)) void exc28(interruptFrame *frame)
        {
            kernel_panic(frame, exceptions[28], exceptionNoError);
            stop();
        }

        // Exceptions with error codes

        __attribute__((interrupt)) void exc8(interruptFrame *frame, uint64_t error)
        {
            kernel_panic(frame, exceptions[8], exceptionError, error, true);
            stop();
        }
        __attribute__((interrupt)) void exc10(interruptFrame *frame, uint64_t error)
        {
            kernel_panic(frame, exceptions[10], exceptionError, error, true);
            stop();
        }
        __attribute__((interrupt)) void exc11(interruptFrame *frame, uint64_t error)
        {
            kernel_panic(frame, exceptions[11], exceptionError, error, true);
            stop();
        }
        __attribute__((interrupt)) void exc12(interruptFrame *frame, uint64_t error)
        {
            kernel_panic(frame, exceptions[12], exceptionError, error, true);
            stop();
        }
        __attribute__((interrupt)) void exc13(interruptFrame *frame, uint64_t error)
        {
            kernel_panic(frame, exceptions[13], exceptionError, error, true);
            stop();
        }
        __attribute__((interrupt)) void exc14(interruptFrame *frame, uint64_t error)
        {
            kernel_panic(frame, exceptions[14], exceptionError, error, true);
            stop();
        }
        __attribute__((interrupt)) void exc17(interruptFrame *frame, uint64_t error)
        {
            kernel_panic(frame, exceptions[17], exceptionError, error, true);
            stop();
        }
        __attribute__((interrupt)) void exc21(interruptFrame *frame, uint64_t error)
        {
            kernel_panic(frame, exceptions[21], exceptionError, error, true);
            stop();
        }
        __attribute__((interrupt)) void exc29(interruptFrame *frame, uint64_t error)
        {
            kernel_panic(frame, exceptions[29], exceptionError, error, true);
            stop();
        }
        __attribute__((interrupt)) void exc30(interruptFrame *frame, uint64_t error)
        {
            kernel_panic(frame, exceptions[30], exceptionError, error, true);
            stop();
        }

        int64_t countdown = 0;

        __attribute__((interrupt)) void timer_handler(interruptFrame*)
        {
            countdown--;
            millis_since_boot++;
            stdlib::update_timers();
            apicSendEOI();
            return; // iretq
        }

        __attribute__((interrupt)) void spurious_isr(interruptFrame*)
        {
            log.e("Spurious ISR", "????????????");
            apicSendEOI();
            return; // iretq
        }
    }

    void sleep(int64_t millis)
    {
        countdown = millis;
        while (countdown > 0)
            ;
    }

    void fill_idt(void)
    {

        remapPIC(0xF0, 0xF8);

        // Exceptions

        setIDTEntry(0, (void *)exc0, EXCEPTION);
        setIDTEntry(1, (void *)exc1, EXCEPTION);
        setIDTEntry(2, (void *)exc2, EXCEPTION);
        setIDTEntry(3, (void *)exc3, EXCEPTION);
        setIDTEntry(4, (void *)exc4, EXCEPTION);
        setIDTEntry(5, (void *)exc5, EXCEPTION);
        setIDTEntry(6, (void *)exc6, EXCEPTION);
        setIDTEntry(7, (void *)exc7, EXCEPTION);
        setIDTEntry(8, (void *)exc8, EXCEPTION);
        setIDTEntry(9, (void *)exc9, EXCEPTION);
        setIDTEntry(10, (void *)exc10, EXCEPTION);
        setIDTEntry(11, (void *)exc11, EXCEPTION);
        setIDTEntry(12, (void *)exc12, EXCEPTION);
        setIDTEntry(13, (void *)exc13, EXCEPTION);
        setIDTEntry(14, (void *)exc14, EXCEPTION);
        setIDTEntry(16, (void *)exc16, EXCEPTION);
        setIDTEntry(17, (void *)exc17, EXCEPTION);
        setIDTEntry(18, (void *)exc18, EXCEPTION);
        setIDTEntry(19, (void *)exc19, EXCEPTION);
        setIDTEntry(20, (void *)exc20, EXCEPTION);
        setIDTEntry(21, (void *)exc21, EXCEPTION);
        setIDTEntry(28, (void *)exc28, EXCEPTION);
        setIDTEntry(29, (void *)exc29, EXCEPTION);
        setIDTEntry(30, (void *)exc30, EXCEPTION);

        for (int i = 32; i < 256; i++)
        {
            setIDTEntry(i, (void *)exc1, EXCEPTION);
        }

        setIDTEntry(0x20, (void *)timer_handler, INTERRUPT);
        setIDTEntry(0x27, (void *)spurious_isr, INTERRUPT);

        lidt(IDT, sizeof(IDT) - 1);

        asm("sti");
    }
}
