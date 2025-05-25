#include <types.h>
#include <limine/limine.h>
#include <kernel/x64/io.h>

namespace serial
{
    static constexpr uint16_t COM1 = 0x3F8;
    static constexpr uint16_t COM2 = 0x2F8;

    void serial_write(limine::limine_terminal *, const char *str, uint64_t len)
    {
        for (int i = 0; i < len; i++)
        {
            kernel::outb(COM1, str[i]);
        }
    }
}