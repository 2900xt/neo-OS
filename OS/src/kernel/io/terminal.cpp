#include <kernel/kernel.h>
#include <drivers/vga/vga.h>
#include <drivers/vga/fonts.h>

namespace vga
{
    extern PSF_header_t *font_hdr;
}

namespace kernel
{
    int terminal_cols;
    int terminal_rows;
    int terminal_x = 0, terminal_y = 0;
    stdlib::spinlock_t lock = stdlib::SPIN_UNLOCKED;
    char* input_buffer;
    int input_pos = 0;

    void draw_character(int x, int y, char c, bool is_input = false)
    {
        int vga_x = x * vga::font_hdr->width;
        int vga_y = y * vga::font_hdr->height;

        vga::putchar(vga_x, vga_y, c);
    
        if (is_input)
        {
            input_buffer[input_pos++] = c;
            input_buffer[input_pos] = '\0';
        }
    }

    void draw_cursor(int x, int y)
    {
        x *= vga::font_hdr->width;
        y *= vga::font_hdr->height;

        vga::fillRect(x, y, {255, 255, 255}, vga::font_hdr->width, vga::font_hdr->height);
    }

    void terminal_putc(char src, bool is_input = false)
    {
        switch (src)
        {
            case '\n':
                //remove cursor
                draw_character(terminal_x, terminal_y, ' ');

                terminal_y++;
                terminal_x = 0;

                //draw cursor
                draw_cursor(terminal_x, terminal_y);
                break;
            case '\t':
                //remove cursor
                draw_character(terminal_x, terminal_y, ' ');

                terminal_x += 4 - (terminal_x % 4);

                //draw cursor
                draw_cursor(terminal_x, terminal_y);

                if (is_input)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        input_buffer[input_pos + i] = ' ';
                    }

                    input_pos += 4 - (input_pos % 4);
                    input_buffer[input_pos] = '\0';
                }

                break;
            case '\b':
                if (terminal_x > 0)
                {
                    //remove cursor
                    draw_character(terminal_x, terminal_y, ' ');

                    terminal_x--;

                    //draw cursor
                    draw_cursor(terminal_x, terminal_y);

                    if (is_input)
                    {
                        input_pos--;
                        input_buffer[input_pos] = '\0';
                    }
                }
                break;
            default:
                draw_character(terminal_x, terminal_y, src, is_input);

                //draw cursor
                terminal_x++;
                draw_cursor(terminal_x, terminal_y);
                break;
        }
    }

    void terminal_puts(const char *src)
    {
        while (*src != '\0')
        {
            terminal_putc(*src++);
        }
    }

    void terminal_init()
    {
        terminal_cols = vga::fbuf_info->width / vga::font_hdr->width;
        terminal_rows = vga::fbuf_info->height / vga::font_hdr->height;
        input_buffer = (char*)kernel::kmalloc(terminal_cols * terminal_rows + 69);
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

}