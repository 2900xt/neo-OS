#include "stdlib/lock.h"
#include <stdlib/stdlib.h>
#include <kernel/proc/stream.h>
#include <drivers/vga/vga.h>
#include <drivers/vga/fonts.h>
#include <stdarg.h>

extern stream *kernel_stdout, *kernel_stdin;
namespace VGA {
extern PSF_header_t* font_hdr;
int terminal_x = 0, terminal_y = 0;
}

namespace std
{

int terminal_cols = VGA::fbuf_info->width / VGA::font_hdr->width;
int terminal_rows = VGA::fbuf_info->height / VGA::font_hdr->height;
spinlock_t lock = false;

void terminal_putc(char src)
{
    stream_write(kernel_stdout, src);
}

void terminal_puts(const char* src)
{
    while(*src != '\0')
    {
        terminal_putc(*src++);
    }
}

void        *token;
uint64_t    num;
double      dec;
int         current;
bool        argFound;

void printf(const char* fmt, ...)
{
    acquire_spinlock(&lock);
    va_list args;
    va_start(args, fmt);

    while(fmt[current] != '\0')
    {
    
        if(fmt[current] != '%')
        {
            terminal_putc(fmt[current++]);
            continue;
        }
        
        switch(fmt[current + 1])
        {
            case 'c':   //Char
            case 'C':
                argFound = true;
                num = va_arg(args, int);
                terminal_putc(num);
                break;
            case 's':   //String
            case 'S':
                argFound = true;
                token = va_arg(args, void*);
                terminal_puts((char*)token);
                break;
            case 'u':   //Unsigned Integer
            case 'U':
                argFound = true;
                num = va_arg(args, uint64_t);
                token = utoa(num, 10);
                terminal_puts((char*)token);
                break;
            case 'd':
            case 'D':   //Signed integer
                argFound = true;
                num = va_arg(args, int64_t);
                token = itoa(num, 10);
                terminal_puts((char*)token);
                break;
            case 'x':   //Hexadecimal unsigned int
            case 'X':
                argFound = true;
                num = va_arg(args, uint64_t);
                token = utoa(num, 16);
                terminal_puts((char*)token);
                break;
            case 'b':   //Binary
            case 'B':
                argFound = true;
                num = va_arg(args, uint64_t);
                token = utoa(num, 2);
                terminal_puts((char*)token);
                break;
            case 'f':
            case 'F':
                dec = va_arg(args, double);
                token = dtoa(dec, 5);
                argFound = true;

                terminal_puts((char*)token);

                kfree(token);
                break;
            default:
                argFound = false;
        }

        if(argFound)
        {
            current += 2;
            continue;
        }

        current++;
    }

    va_end(args);
    release_spinlock(&lock);

}

void update_terminal()
{
    if(kernel_stdout->ack_update)
    {
        std::string *data = stream_read(kernel_stdout);
        VGA::putstring(data->c_str());
        VGA::repaintScreen();
        stream_flush(kernel_stdout);
    }
}

}