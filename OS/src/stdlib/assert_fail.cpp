#include <kernel/io/log.h>


namespace stdlib
{
    void assert_fail(const char *assertion, const char *file, unsigned int line)
    {
        log.e(kernel::kernel_tag, "Assertion Failed -> %s: in file %s, line %u", assertion, file, line);
        for (;;)
            ;
    }
}