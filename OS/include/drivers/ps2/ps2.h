#ifndef PS2_H
#define PS2_H

#include <types.h>

namespace ps2
{
    bool pollKeyInput();
    extern uint8_t lastKey;
}

#endif