#ifndef SCAN_H
#define SCAN_H

#include <stdlib/string.h>

namespace kernel
{
    char pollNextChar();
    void getNextLine(stdlib::string *output);
}

#endif