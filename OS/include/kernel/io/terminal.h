#ifndef TERMINAL_H
#define TERMINAL_H

#include <stdlib/stdlib.h>

namespace kernel
{
    void terminal_init();
    void terminal_putc(char src, bool is_input = false);
    void terminal_puts(const char *src);
}

#endif