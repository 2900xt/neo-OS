#ifndef PS2_H
#define PS2_H

namespace ps2
{
    bool pollKeyInput();
    extern uint8_t lastKey;
    extern bool shiftBit;
    extern uint8_t lastSpecialKey;
    
    // Mouse support
    struct MouseState {
        int x, y;
        bool left_button;
        bool right_button;
        bool middle_button;
        int delta_x, delta_y;
    };
    
    bool pollMouseInput();
    void mouse_init();
    extern MouseState mouse_state;
}

#endif