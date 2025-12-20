#include "kernel/x64/intr/idt.h"
#include <kernel/io/log.h>
#include "kernel/proc/smp.h"

namespace stdlib
{
    void assert_fail(const char *assertion, const char *file, unsigned int line)
    {
        static char buffer[1024];
        sprintf(buffer, "Assertion '%s' failed in file %s at line %d", assertion, file, line);
        log.e(kernel::kernel_tag, "Assertion Failed -> %s", buffer);
        stdlib::string message(buffer);
        kernel::panic(message);
        for (;;)
            ;
    }
}