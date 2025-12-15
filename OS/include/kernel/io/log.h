#pragma once

#include <types.h>
#include <stdarg.h>
#include <limine/limine.h>

namespace kernel
{

    extern limine::limine_terminal *console;
    extern limine::limine_terminal_write write;
    void log_init(void);

    [[gnu::deprecated]]
    inline void klogf(...)
    {
        return;
    }

    extern const char *kernel_tag;
}

// Serial Kernel Logger
class serial_logger
{
public:
    void v(const char *tag, const char *fmt, ...);
    void d(const char *tag, const char *fmt, ...);
    void w(const char *tag, const char *fmt, ...);
    void e(const char *tag, const char *fmt, ...);
};

extern serial_logger log;
