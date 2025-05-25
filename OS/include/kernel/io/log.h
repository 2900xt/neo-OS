#pragma once

#include <types.h>
#include <stdarg.h>
#include <limine/limine.h>

namespace kernel
{

    extern limine::limine_terminal *console;
    extern limine::limine_terminal_write write;
    void tty_init(void);

    [[gnu::deprecated]]
    inline void klogf(...)
    {
        return;
    }

    void update_terminal();

    void printf(const char *fmt, ...);

    extern const char *kernel_tag;
}

// Serial Kernel Logger
class log
{
public:
    static void v(const char *tag, const char *fmt, ...);
    static void d(const char *tag, const char *fmt, ...);
    static void w(const char *tag, const char *fmt, ...);
    static void e(const char *tag, const char *fmt, ...);
};
