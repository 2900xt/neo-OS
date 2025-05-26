#include <kernel/kernel.h>
#include <drivers/ps2/ps2.h>
#include <drivers/vga/fonts.h>

namespace kernel
{
    // milliseconds
    constexpr uint64_t pollDelay = 10;
    bool outputEnabled = true;

    char getNextChar()
    {
        while (!ps2::pollKeyInput())
        {
            kernel::sleep(pollDelay);
        }

        if (outputEnabled)
        {
            char buf[2];
            buf[0] = ps2::lastKey;
            buf[1] = '\0';
            vga::putstring(buf);
        }

        return ps2::lastKey;
    }

    void getNextLine(stdlib::string *output)
    {
        output->clear();
        while (true)
        {
            char c = getNextChar();
            output->push_back(c);
            if (c == '\n' || c == '\r' || c == '\0')
                break;
        }
    }
}