
#include <drivers/ps2/ps2.h>
#include <drivers/vga/fonts.h>
#include <stdlib/string.h>
#include <kernel/io/terminal.h>

namespace kernel
{
    // milliseconds
    constexpr uint64_t pollDelay = 10;
    bool outputEnabled = true;

    char pollNextChar()
    {
        if (!ps2::pollKeyInput())
        {
            return 0xFF;
        }

        if (outputEnabled)
        {
            terminal_putc(ps2::lastKey, true);
        }

        return ps2::lastKey;
    }

    void getNextLine(stdlib::string *output)
    {
        output->clear();
        while (true)
        {
            char c = pollNextChar();

            if (c == 0xFF)
                continue;

            output->push_back(c);

            if (c == '\n' || c == '\r' || c == '\0')
                break;
        }
    }
}