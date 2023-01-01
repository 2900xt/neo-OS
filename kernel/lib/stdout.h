#include <types.h>
#include <stdarg.h>
#include <limine/limine.h>

#ifndef STDOUT_H
#define STDOUT_H

namespace neoOS_STD
{
    extern limine::limine_terminal* console;
    extern limine::limine_terminal_write write;

    size_t strlen(char*);
    char* itoa(int64_t, uint8_t);
    void puts(char*);
    void putc(char);
    void printf(const char*, ...);
    void done(void);
}


#endif // !STDOUT_H