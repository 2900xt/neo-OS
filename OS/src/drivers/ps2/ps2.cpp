#include <types.h>
#include <kernel/x64/io.h>
#include <drivers/vga/vga.h>
#include <stdlib/stdlib.h>
#include <kernel/kernel.h>
#include <drivers/ps2/ps2.h>

namespace ps2
{

    enum PS2_KEYBOARD_CODES : uint8_t
    {
        KEY_ESC_PRESS = 0x01,
        KEY_ENTER_PRESS = 0x1C,
        KEY_LCTRL_PRESS = 0x1D,
        KEY_LSHIFT_PRESS = 0x2A,
        KEY_RSHIFT_PRESS = 0x36,
        KEY_LALT_PRESS = 0x38,
        KEY_CAPS_PRESS = 0x3A,
        KEY_CAPS_RELEASE = 0xBA,
        KEY_LSHIFT_RELEASE = 0xAA,
        KEY_RSHIFT_RELEASE = 0xB6,
    };

    const uint8_t ScanCodeLookupTable[] = {
        0, 0, '1', '2',
        '3', '4', '5', '6',
        '7', '8', '9', '0',
        '-', '=', '\b', '\t',
        'q', 'w', 'e', 'r',
        't', 'y', 'u', 'i',
        'o', 'p', '[', ']',
        '\n', 0, 'a', 's',
        'd', 'f', 'g', 'h',
        'j', 'k', 'l', ';',
        '\'', '`', 0, '\\',
        'z', 'x', 'c', 'v',
        'b', 'n', 'm', ',',
        '.', '/', 0, '*',
        0, ' '};

    const uint8_t ScanCodeLookupTableShift[] = {
        0, 0, '!', '@',
        '#', '$', '%', '^',
        '&', '*', '(', ')',
        '_', '+', '\b', '\t',
        'Q', 'W', 'E', 'R',
        'T', 'Y', 'U', 'I',
        'O', 'P', '{', '}',
        '\n', 0, 'A', 'S',
        'D', 'F', 'G', 'H',
        'J', 'K', 'L', ':',
        '"', '~', 0, '|',
        'Z', 'X', 'C', 'V',
        'B', 'N', 'M', '<',
        '>', '?', 0, '*',
        0, ' '};

    uint8_t lastKey = 0;
    bool shiftBit = false;
    
    // Mouse state
    MouseState mouse_state = {0, 0, false, false, false, 0, 0};
    static uint8_t mouse_packet[3];
    static int mouse_packet_index = 0;

    enum PS2_REGS
    {
        PS2_DATA = 0x60,
        PS2_STATUS = 0x64,
        PS2_CMD = 0x64,
    };

    enum PS2_STATUS_FLAGS
    {
        OUTPUT_ALLOWED = (1 << 0),
        INPUT_ALLOWED = (1 << 1),
        CONTROLLER_DATA = (1 << 3),
    };

    bool pollKeyInput()
    {
        if (~kernel::inb(PS2_STATUS) & OUTPUT_ALLOWED)
        {
            return false;
        }

        uint8_t code = kernel::inb(0x60);

        if (code == KEY_LSHIFT_PRESS || code == KEY_RSHIFT_PRESS)
        {
            shiftBit = true;
        }
        else if (code == KEY_LSHIFT_RELEASE || code == KEY_RSHIFT_RELEASE)
        {
            shiftBit = false;
        }

        if (code < 59)
        {
            lastKey = (!shiftBit ? ScanCodeLookupTable[code] : ScanCodeLookupTableShift[code]);
            // log::e("PS/2 Driver", "Key pressed: %c", lastKey);
            return true;
        }

        return false;
    }

    void waitForInput(void)
    {
        while (!(~kernel::inb(PS2_STATUS) & INPUT_ALLOWED))
            ;
    }
    
    void waitForOutput(void)
    {
        while (!(kernel::inb(PS2_STATUS) & OUTPUT_ALLOWED))
            ;
    }
    
    void mouse_write(uint8_t data)
    {
        waitForInput();
        kernel::outb(PS2_CMD, 0xD4);
        waitForInput();
        kernel::outb(PS2_DATA, data);
    }
    
    uint8_t mouse_read()
    {
        waitForOutput();
        return kernel::inb(PS2_DATA);
    }
    
    void mouse_init()
    {
        // Enable auxiliary device (mouse)
        waitForInput();
        kernel::outb(PS2_CMD, 0xA8);
        
        // Enable mouse interrupts
        waitForInput();
        kernel::outb(PS2_CMD, 0x20);
        waitForOutput();
        uint8_t status = kernel::inb(PS2_DATA) | 2;
        waitForInput();
        kernel::outb(PS2_CMD, 0x60);
        waitForInput();
        kernel::outb(PS2_DATA, status);
        
        // Set mouse to use default settings
        mouse_write(0xF6);
        mouse_read(); // Read ACK
        
        // Enable mouse data reporting
        mouse_write(0xF4);
        mouse_read(); // Read ACK
        
        // Initialize mouse position to center of screen
        if (vga::fbuf_info) {
            mouse_state.x = vga::fbuf_info->width / 2;
            mouse_state.y = vga::fbuf_info->height / 2;
        }
        
        log::d("PS/2", "Mouse initialized");
    }
    
    bool pollMouseInput()
    {
        if (~kernel::inb(PS2_STATUS) & OUTPUT_ALLOWED)
        {
            return false;
        }
        
        // Check if data is from auxiliary device (mouse)
        if (~kernel::inb(PS2_STATUS) & CONTROLLER_DATA)
        {
            return false;
        }
        
        uint8_t data = kernel::inb(PS2_DATA);
        
        // Collect 3-byte mouse packet
        mouse_packet[mouse_packet_index] = data;
        mouse_packet_index++;
        
        if (mouse_packet_index >= 3)
        {
            mouse_packet_index = 0;
            
            // Parse mouse packet
            uint8_t flags = mouse_packet[0];
            int8_t delta_x = mouse_packet[1];
            int8_t delta_y = mouse_packet[2];
            
            // Check if packet is valid (bit 3 should be set)
            if (!(flags & 0x08))
            {
                return false;
            }
            
            // Update button states
            mouse_state.left_button = flags & 0x01;
            mouse_state.right_button = flags & 0x02;
            mouse_state.middle_button = flags & 0x04;
            
            // Update deltas
            mouse_state.delta_x = delta_x;
            mouse_state.delta_y = -delta_y; // Invert Y for screen coordinates
            
            // Update absolute position with bounds checking
            if (vga::fbuf_info) {
                mouse_state.x += delta_x;
                mouse_state.y += -delta_y; // Invert Y for screen coordinates
                
                // Clamp to screen bounds
                if (mouse_state.x < 0) mouse_state.x = 0;
                if (mouse_state.y < 0) mouse_state.y = 0;
                if (mouse_state.x >= (int)vga::fbuf_info->width) mouse_state.x = vga::fbuf_info->width - 1;
                if (mouse_state.y >= (int)vga::fbuf_info->height) mouse_state.y = vga::fbuf_info->height - 1;
            }
            
            return true;
        }
        
        return false;
    }
}
