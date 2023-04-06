#include <stdlib/stdio.h>


void assert_fail(const char *assertion, const char *file, unsigned int line)
{
    std::klogf("Assertion Failed -> %s: in file %s, line %u\n", assertion , file, line);
    for(;;);
}