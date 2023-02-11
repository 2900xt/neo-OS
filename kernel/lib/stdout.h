#ifndef STDOUT_H
#define STDOUT_H

#include <types.h>
#include <stdarg.h>
#include <limine/limine.h>


enum klog_levels
{
    LOG_CRITICAL = 0,
    LOG_ERROR = 1,
    LOG_WARNING = 2,
    LOG_IMPORTANT = 3,
    LOG_DEBUG = 4,
};

extern limine::limine_terminal* console;
extern limine::limine_terminal_write write;
size_t      strlen(const char*);
char* itoa(uint64_t val, uint8_t radix);
void        puts(char*);
void        putc(char);
void        klogf(int level, const char*, ...);
void        bsp_done(void);
bool        strcmp(const char* a, const char* b, int count);
uint64_t    max(uint64_t a, uint64_t b);
uint64_t    min(uint64_t a, uint64_t b);
double      get_boot_time();

#endif // !STDOUT_H
