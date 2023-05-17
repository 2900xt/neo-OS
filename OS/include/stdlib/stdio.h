#pragma once

#include <types.h>
#include <stdarg.h>
#include <limine/limine.h>


namespace std 
{

extern limine::limine_terminal* console;
extern limine::limine_terminal_write write;
void        puts(const char*);
void        puts_16(const char*);
void        putc(char);
void        tty_init(void);

[[gnu::deprecated]]
inline void klogf(...)
{
    return;
}
}

//Serial Kernel Logger
class Logger 
{
    public:

    void v(const char * tag, const char * fmt, ...);
    void d(const char * tag, const char * fmt, ...);
    void w(const char * tag, const char * fmt, ...);
    void e(const char * tag, const char * fmt, ...);
};

extern const char * kernel_tag;
extern Logger Log;