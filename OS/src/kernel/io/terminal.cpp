#include <kernel/kernel.h>
#include <drivers/vga/vga.h>
#include <drivers/vga/fonts.h>

namespace vga
{
    extern PSF_header_t *font_hdr;
    int terminal_x = 0, terminal_y = 0;
}

namespace kernel
{
    extern stream *kernel_stdout, *kernel_stdin;
    int terminal_cols = vga::fbuf_info->width / vga::font_hdr->width;
    int terminal_rows = vga::fbuf_info->height / vga::font_hdr->height;
    stdlib::spinlock_t lock = stdlib::SPIN_UNLOCKED;

    void terminal_putc(char src)
    {
        kernel::stream_write(kernel_stdout, src);
    }

    void terminal_puts(const char *src)
    {
        while (*src != '\0')
        {
            terminal_putc(*src++);
        }
    }

    void *token;
    uint64_t num;
    double dec;
    int current;
    bool argFound;

    void printf(const char *fmt, ...)
    {
        stdlib::acquire_spinlock(&lock);
        va_list args;
        va_start(args, fmt);

        while (fmt[current] != '\0')
        {

            if (fmt[current] != '%')
            {
                terminal_putc(fmt[current++]);
                continue;
            }

            switch (fmt[current + 1])
            {
            case 'c': // Char
            case 'C':
                argFound = true;
                num = va_arg(args, int);
                terminal_putc(num);
                break;
            case 's': // String
            case 'S':
                argFound = true;
                token = va_arg(args, void *);
                terminal_puts((char *)token);
                break;
            case 'u': // Unsigned Integer
            case 'U':
                argFound = true;
                num = va_arg(args, uint64_t);
                token = stdlib::utoa(num, 10);
                terminal_puts((char *)token);
                break;
            case 'd':
            case 'D': // Signed integer
                argFound = true;
                num = va_arg(args, int64_t);
                token = stdlib::itoa(num, 10);
                terminal_puts((char *)token);
                break;
            case 'x': // Hexadecimal unsigned int
            case 'X':
                argFound = true;
                num = va_arg(args, uint64_t);
                token = stdlib::utoa(num, 16);
                terminal_puts((char *)token);
                break;
            case 'b': // Binary
            case 'B':
                argFound = true;
                num = va_arg(args, uint64_t);
                token = stdlib::utoa(num, 2);
                terminal_puts((char *)token);
                break;
            case 'f':
            case 'F':
                dec = va_arg(args, double);
                token = stdlib::dtoa(dec, 5);
                argFound = true;

                terminal_puts((char *)token);

                kernel::kfree(token);
                break;
            default:
                argFound = false;
            }

            if (argFound)
            {
                current += 2;
                continue;
            }

            current++;
        }

        va_end(args);
        stdlib::release_spinlock(&lock);
    }

    void update_terminal()
    {
        if (kernel_stdout->ack_update)
        {
            stdlib::string *data = kernel::stream_read(kernel_stdout);
            vga::putstring(data->c_str(), vga::terminal_x, vga::terminal_y);
            vga::repaintScreen();
            kernel::stream_flush(kernel_stdout);
        }
    }

}