#pragma once

#include <types.h>
#include <stdarg.h>
#include <limine/limine.h>


namespace std 
{

extern limine::limine_terminal* console;
extern limine::limine_terminal_write write;
void        puts(const char*);
void        putc(char);
void        klogf(const char*, ...);
void        tty_init(void);

}