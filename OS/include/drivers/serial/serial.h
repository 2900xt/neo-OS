#include <types.h>
#include <limine/limine.h>
#include <kernel/x64/io.h>

namespace serial
{
    void serial_write(limine::limine_terminal *, const char *str, uint64_t len);
}