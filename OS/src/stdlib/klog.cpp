#include "stdlib/stdlib.h"
#include "stdlib/string.h"
#include <stdarg.h>
#include <types.h>
#include <config.h>
#include <limine/limine.h>
#include <stdlib/stdlib.h>
#include <stdlib/lock.h>
#include <drivers/serial/serial.h>

static volatile limine::limine_terminal_request terminal_request = {LIMINE_TERMINAL_REQUEST, 0};

namespace std 
{

limine::limine_terminal* console;
limine::limine_terminal_write write;
static bool serial_output;

void tty_init(void)
{
    if (terminal_request.response == NULL || terminal_request.response->terminal_count == 0)
    {
        write = (limine::limine_terminal_write)SERIAL::serial_write;
        console = NULL;
        serial_output = true;
        return;
    }

    write = (SERIAL_OUTPUT_ENABLE ? (limine::limine_terminal_write)SERIAL::serial_write : terminal_request.response->write);
    console = (SERIAL_OUTPUT_ENABLE ? NULL : terminal_request.response->terminals[0]);
    serial_output = SERIAL_OUTPUT_ENABLE;
}

void puts(const char *src)
{
    write(console, src, strlen(src));
}

void putc(char c)
{
    write(console, &c, 1);
    if(c == '\n') 
    {
        putc('\r');
    }
}

//Print unicode-16
void puts_16(uint16_t *src)
{
    while(*src++)
    {
        putc((char)*src);
        src++;
    }
}


static struct klog_state_t
{
    void       *token;
    uint64_t    num;
    double      dec;

    int         current;
    bool        argFound;
    spinlock_t  lock = false;
} klog_state;

static void klogvf(const char* fmt, va_list args)
{
    acquire_spinlock(&klog_state.lock);

    klog_state.current = 0;
    klog_state.argFound = false;

    while(fmt[klog_state.current] != '\0')
    {
    
        if(fmt[klog_state.current] != '%')
        {
            putc(fmt[klog_state.current++]);
            continue;
        }

        switch(fmt[klog_state.current + 1])
        {
            case 'c':   //Char
            case 'C':
                klog_state.argFound = true;
                klog_state.num = va_arg(args, int);
                putc(klog_state.num);
                break;
            case 's':   //String
            case 'S':
                klog_state.argFound = true;
                klog_state.token = va_arg(args, void*);
                puts((char*)klog_state.token);
                break;
            case 'u':   //Unsigned Integer
            case 'U':
                klog_state.argFound = true;
                klog_state.num = va_arg(args, uint64_t);
                klog_state.token = utoa(klog_state.num, 10);
                puts((char*)klog_state.token);
                break;
            case 'd':
            case 'D':   //Signed integer
                klog_state.argFound = true;
                klog_state.num = va_arg(args, int64_t);
                klog_state.token = itoa(klog_state.num, 10);
                puts((char*)klog_state.token);
                break;
            case 'x':   //Hexadecimal unsigned int
            case 'X':
                klog_state.argFound = true;
                klog_state.num = va_arg(args, uint64_t);
                klog_state.token = utoa(klog_state.num, 16);
                puts((char*)klog_state.token);
                break;
            case 'b':   //Binary
            case 'B':
                klog_state.argFound = true;
                klog_state.num = va_arg(args, uint64_t);
                klog_state.token = utoa(klog_state.num, 2);
                puts((char*)klog_state.token);
                break;
            case 'l':
            case 'L':   //UTF-16 string
                klog_state.token = va_arg(args, uint16_t*);
                puts_16((uint16_t*)klog_state.token);
                klog_state.argFound = true;
                break;
            case 'f':
            case 'F':
                klog_state.dec = va_arg(args, double);
                klog_state.token = dtoa(klog_state.dec, 5);
                klog_state.argFound = true;

                puts((char*)klog_state.token);

                kfree(klog_state.token);
                break;
            default:
                klog_state.argFound = false;
        }

        if(klog_state.argFound)
        {
            klog_state.current += 2;
            continue;
        }

        klog_state.current++;
    }

    release_spinlock(&klog_state.lock);
}

}

extern uint64_t millis_since_boot;
Logger Log;

void Logger::v(const char *source, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char *seconds = std::dtoa(millis_since_boot / 1000.0, 5);
    std::puts(seconds);
    delete []seconds;

    std::puts("\t[Verbose]   ");
    std::puts(source);
    int len = std::strlen(source);
    for(int i = len; i <= 15; i++)
    {
        std::putc(' ');
    }

    std::klogvf(fmt, args);

    std::putc('\n');

    va_end(args);
}

void Logger::d(const char *source, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char *seconds = std::dtoa(millis_since_boot / 1000.0, 5);
    std::puts(seconds);
    delete []seconds;
    
    std::puts("\t[Debug]     ");
    std::puts(source);
    int len = std::strlen(source);
    for(int i = len; i <= 15; i++)
    {
        std::putc(' ');
    }

    std::klogvf(fmt, args);
    std::putc('\n');

    va_end(args);
}

void Logger::w(const char *source, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char *seconds = std::dtoa(millis_since_boot / 1000.0, 5);
    std::puts(seconds);
    delete []seconds;
    
    std::puts("\t[Warning]   ");
    std::puts(source);
    int len = std::strlen(source);
    for(int i = len; i <= 15; i++)
    {
        std::putc(' ');
    }

    std::klogvf(fmt, args);
    std::putc('\n');

    va_end(args);
}

void Logger::e(const char *source, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
   
    char *seconds = std::dtoa(millis_since_boot / 1000.0, 5);
    std::puts(seconds);
    delete []seconds;

    std::puts("\t[Error]     ");
    std::puts(source);
    
    int len = std::strlen(source);
    for(int i = len; i <= 15; i++)
    {
        std::putc(' ');
    }

    std::klogvf(fmt, args);
    std::putc('\n');

    va_end(args);
}