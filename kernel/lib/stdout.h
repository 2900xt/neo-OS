#include <types.h>
#include <stdarg.h>
#include <limine/limine.h>

#ifndef STDOUT_H
#define STDOUT_H

namespace neoSTL
{
    extern limine::limine_terminal* console;
    extern limine::limine_terminal_write write;

    size_t strlen(char*);
    char* itoa(int64_t, uint8_t);
    void puts(char*);
    void putc(char);
    void printf(const char*, ...);
    void done(void);
    bool strcmp(const char* a, const char* b, int count);

    uint64_t max(uint64_t a, uint64_t b);
    uint64_t min(uint64_t a, uint64_t b);
}


#endif // !STDOUT_H