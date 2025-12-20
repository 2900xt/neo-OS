#include <types.h>
#include <limine/limine.h>
#include <kernel/io/log.h>
#include <kernel/x64/intr/apic.h>
#include <kernel/proc/smp.h>
#include <drivers/vga/vga.h>
#include <drivers/vga/fonts.h>
#include "drivers/vga/fonts.h"
#include "drivers/vga/vga.h"
#include "kernel/shell/shell.h"
#include <stdlib/structures/string.h>
#include <drivers/vga/fonts.h>

namespace vga {
    extern PSF_header_t *font_hdr;
};

namespace kernel
{

    volatile limine::limine_smp_request smp_request = {LIMINE_SMP_REQUEST, 0, NULL, NULL};

    const char *kernel_scheduler_tag = "Scheduler";

    void smp_init(void)
    {
        kernel::initAPIC(smp_request.response->bsp_lapic_id);
        log.d(kernel_scheduler_tag, "%u CPU cores detected", smp_request.response->cpu_count);

        for (int i = 1; i < smp_request.response->cpu_count; i++)
        {
            cpu_jump_to(i, (void *)mt_begin);
            kernel::sleep(100);
        }
    }

    void mt_begin(limine::limine_smp_info *cpu_info)
    {
        kernel::initAPIC(cpu_info->lapic_id);
        for (;;)
        {
            asm("hlt");
        }
    }

    void cpu_jump_to(uint8_t pid, void *addr)
    {
        int currentCPU = 0;
        limine::limine_smp_info *cpu;
        while (currentCPU <= smp_request.response->cpu_count)
        {
            cpu = smp_request.response->cpus[currentCPU];
            if (cpu->processor_id == pid)
            {
                cpu->goto_address = (limine::limine_goto_address)addr;
                return;
            }
            currentCPU++;
        }

        log.e(kernel_scheduler_tag, "Unable to find CPU #%u!", pid);
    }

    extern int terminal_x, terminal_y;
    void panic(stdlib::string message)
    {
        uint64_t stack[8];
        asm volatile(
            "mov %%rax, %0\n"
            "mov %%rbx, %1\n"
            "mov %%rcx, %2\n"
            "mov %%rdx, %3\n"
            "mov %%rsi, %4\n"
            "mov %%rdi, %5\n"
            "mov %%rbp, %6\n"
            "mov %%rsp, %7\n"
            : "=m"(stack[0]), "=m"(stack[1]), "=m"(stack[2]), "=m"(stack[3]),
              "=m"(stack[4]), "=m"(stack[5]), "=m"(stack[6]), "=m"(stack[7])
        );
        
        //panic to serial first
        log.e("KERNEL PANIC", "Exception: %s", message.c_str());
        log.e("KERNEL PANIC", "--- Registers ---");
        log.e("KERNEL PANIC", "RAX: 0x%x", stack[0]);
        log.e("KERNEL PANIC", "RBX: 0x%x", stack[1]);
        log.e("KERNEL PANIC", "RCX: 0x%x", stack[2]);
        log.e("KERNEL PANIC", "RDX: 0x%x", stack[3]);
        log.e("KERNEL PANIC", "RSI: 0x%x", stack[4]);
        log.e("KERNEL PANIC", "RDI: 0x%x", stack[5]);
        log.e("KERNEL PANIC", "RBP: 0x%x", stack[6]);
        log.e("KERNEL PANIC", "RSP: 0x%x", stack[7]);
        log.e("KERNEL PANIC", "--- Stack Trace ---");
        uint64_t *rsp = (uint64_t *)stack[7];
        for (int i = 0; i < 8; i++)
        {
            if (rsp + i == NULL)
                break;
            log.e("KERNEL PANIC", "0x%x", *(rsp + i));
        }

        if(vga::font_hdr == NULL) {
            //if vga isnt initialized, just halt
            for(;;) asm volatile("hlt");
        }

        terminal_x = 0, terminal_y =0;
        // Clear screen to blue (BSOD style)
        vga::fillRect(0, 0, vga::Color(0, 0, 255), vga::fbuf_info->width, vga::fbuf_info->height);
        vga::set_foreground(vga::Color(255, 255, 255));
        vga::set_background(vga::Color(0, 0, 255));

        display_fetch();
        
        // Print to screen using printf
        printf("=== KERNEL PANIC ===\n\n");
        printf("Exception: %s\n", message.c_str());
        
        
        //debug registers manually
        printf("--- Registers ---\n");
        //print each register you got
        printf("RAX: 0x%x\n", stack[0]);
        printf("RBX: 0x%x\n", stack[1]);
        printf("RCX: 0x%x\n", stack[2]);
        printf("RDX: 0x%x\n", stack[3]);
        printf("RSI: 0x%x\n", stack[4]);
        printf("RDI: 0x%x\n", stack[5]);
        printf("RBP: 0x%x\n", stack[6]);
        printf("RSP: 0x%x\n", stack[7]);


        //get stack trace and print, starting from rsp 
        rsp = (uint64_t *)stack[7];
        printf("--- Stack Trace ---\n");
        for (int i = 0; i < 8; i++)
        {
            if (rsp + i == NULL)
                break;
            printf("0x%x\n", *(rsp + i));
        }
        
        vga::repaintScreen();
    }
}