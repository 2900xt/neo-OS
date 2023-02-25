#pragma once

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
void        puts(char*);
void        putc(char);
void        klogf(int level, const char*, ...);
