#ifndef KERNEL_H
#define KERNEL_H

#include <types.h>
#include <config.h>
#include <kernel/io/log.h>
#include <kernel/mem/mem.h>
#include <kernel/mem/paging.h>
#include <kernel/proc/proc.h>
#include <kernel/proc/stream.h>
#include <kernel/smp/smp.h>
#include <kernel/vfs/file.h>
#include <kernel/x64/intr/idt.h>
#include <kernel/x64/intr/apic.h>
#include <kernel/x64/io.h>
#include <kernel/io/scan.h> 
#include <kernel/io/terminal.h>

namespace kernel
{
    void load_drivers();
    void login_init();
    bool login_check();
    void login_prompt(bool failed = false);
    void terminal_clear();    
    void print_prompt();
    void clear_input_buffer();
    void list_files(const char *path);
    void print_file_contents(const char *path);
    void display_fetch();
}

#endif