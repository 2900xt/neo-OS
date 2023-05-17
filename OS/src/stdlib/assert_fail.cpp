#include <stdlib/stdio.h>


void assert_fail(const char *assertion, const char *file, unsigned int line)
{
    Log.e(kernel_tag, "Assertion Failed -> %s: in file %s, line %u", assertion , file, line);
    for(;;);
}