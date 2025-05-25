#include <kernel/kernel.h>
#include <limine/limine.h>
#include <drivers/serial/serial.h>

static volatile limine::limine_terminal_request terminal_request = {LIMINE_TERMINAL_REQUEST, 0};

namespace kernel
{

    limine::limine_terminal *console;
    limine::limine_terminal_write write;
    static bool serial_output;

    void tty_init(void)
    {
        if (terminal_request.response == NULL || terminal_request.response->terminal_count == 0)
        {
            write = (limine::limine_terminal_write)serial::serial_write;
            console = NULL;
            serial_output = true;
            return;
        }

        write = (SERIAL_OUTPUT_ENABLE ? (limine::limine_terminal_write)serial::serial_write : terminal_request.response->write);
        console = (SERIAL_OUTPUT_ENABLE ? NULL : terminal_request.response->terminals[0]);
        serial_output = SERIAL_OUTPUT_ENABLE;
    }

    void puts(const char *src)
    {
        write(console, src, stdlib::strlen(src));
    }

    void putc(char c)
    {
        write(console, &c, 1);
        if (c == '\n')
        {
            putc('\r');
        }
    }

    // Print unicode-16
    void puts_16(uint16_t *src)
    {
        while (*src++)
        {
            putc((char)*src);
            src++;
        }
    }

    static struct klog_state_t
    {
        void *token;
        uint64_t num;
        double dec;

        int current;
        bool argFound;
        stdlib::spinlock_t lock = stdlib::SPIN_UNLOCKED;
    } klog_state;

    static void klogvf(const char *fmt, va_list args)
    {
        stdlib::acquire_spinlock(&klog_state.lock);

        klog_state.current = 0;
        klog_state.argFound = false;

        while (fmt[klog_state.current] != '\0')
        {

            if (fmt[klog_state.current] != '%')
            {
                putc(fmt[klog_state.current++]);
                continue;
            }

            switch (fmt[klog_state.current + 1])
            {
            case 'c': // Char
            case 'C':
                klog_state.argFound = true;
                klog_state.num = va_arg(args, int);
                putc(klog_state.num);
                break;
            case 's': // String
            case 'S':
                klog_state.argFound = true;
                klog_state.token = va_arg(args, void *);
                puts((char *)klog_state.token);
                break;
            case 'u': // Unsigned Integer
            case 'U':
                klog_state.argFound = true;
                klog_state.num = va_arg(args, uint64_t);
                klog_state.token = stdlib::utoa(klog_state.num, 10);
                puts((char *)klog_state.token);
                break;
            case 'd':
            case 'D': // Signed integer
                klog_state.argFound = true;
                klog_state.num = va_arg(args, int64_t);
                klog_state.token = stdlib::itoa(klog_state.num, 10);
                puts((char *)klog_state.token);
                break;
            case 'x': // Hexadecimal unsigned int
            case 'X':
                klog_state.argFound = true;
                klog_state.num = va_arg(args, uint64_t);
                klog_state.token = stdlib::utoa(klog_state.num, 16);
                puts((char *)klog_state.token);
                break;
            case 'b': // Binary
            case 'B':
                klog_state.argFound = true;
                klog_state.num = va_arg(args, uint64_t);
                klog_state.token = stdlib::utoa(klog_state.num, 2);
                puts((char *)klog_state.token);
                break;
            case 'l':
            case 'L': // UTF-16 string
                klog_state.token = va_arg(args, uint16_t *);
                puts_16((uint16_t *)klog_state.token);
                klog_state.argFound = true;
                break;
            case 'f':
            case 'F':
                klog_state.dec = va_arg(args, double);
                klog_state.token = stdlib::dtoa(klog_state.dec, 5);
                klog_state.argFound = true;

                puts((char *)klog_state.token);

                kernel::kfree(klog_state.token);
                break;
            default:
                klog_state.argFound = false;
            }

            if (klog_state.argFound)
            {
                klog_state.current += 2;
                continue;
            }

            klog_state.current++;
        }

        stdlib::release_spinlock(&klog_state.lock);
    }

}

extern uint64_t millis_since_boot;

static void log::v(const char *source, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char *seconds = stdlib::dtoa(millis_since_boot / 1000.0, 5);
    kernel::puts(seconds);
    kernel::kfree(seconds);

    kernel::puts("\t[Verbose]   ");
    kernel::puts(source);
    int len = stdlib::strlen(source);
    for (int i = len; i <= 15; i++)
    {
        kernel::putc(' ');
    }

    kernel::klogvf(fmt, args);

    kernel::putc('\n');

    va_end(args);
}

static void log::d(const char *source, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char *seconds = stdlib::dtoa(millis_since_boot / 1000.0, 5);
    kernel::puts(seconds);
    kernel::kfree(seconds);

    kernel::puts("\t[Debug]     ");
    kernel::puts(source);
    int len = stdlib::strlen(source);
    for (int i = len; i <= 15; i++)
    {
        kernel::putc(' ');
    }

    kernel::klogvf(fmt, args);
    kernel::putc('\n');

    va_end(args);
}

static void log::w(const char *source, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char *seconds = stdlib::dtoa(millis_since_boot / 1000.0, 5);
    kernel::puts(seconds);
    kernel::kfree(seconds);

    kernel::puts("\t[Warning]   ");
    kernel::puts(source);
    int len = stdlib::strlen(source);
    for (int i = len; i <= 15; i++)
    {
        kernel::putc(' ');
    }

    kernel::klogvf(fmt, args);
    kernel::putc('\n');

    va_end(args);
}

static void log::e(const char *source, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);

    char *seconds = stdlib::dtoa(millis_since_boot / 1000.0, 5);
    kernel::puts(seconds);
    kernel::kfree(seconds);

    kernel::puts("\t[Error]     ");
    kernel::puts(source);

    int len = stdlib::strlen(source);
    for (int i = len; i <= 15; i++)
    {
        kernel::putc(' ');
    }

    kernel::klogvf(fmt, args);
    kernel::putc('\n');

    va_end(args);
}