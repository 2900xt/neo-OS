#include <kernel/kernel.h>
#include <stdlib/stdlib.h>

namespace kernel
{
    void shell_init()
    {
        stdlib::string command;
        kernel::getNextLine(&command);
    }
}