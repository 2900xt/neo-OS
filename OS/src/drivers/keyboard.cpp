#include <types.h>
#include <kernel/x64/io.h>
#include <drivers/vga/vga.h>
#include <stdlib/stdlib.h>
#include <kernel/kernel.h>

const char ScanCodeLookupTable[] = {
    0, 0, '1', '2',
    '3', '4', '5', '6',
    '7', '8', '9', '0',
    '-', '=', 0, '\t',
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

uint8_t lastKey = 0;
bool keyPressed = false;
bool shiftBit = false;

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

void keyboardHandler()
{
    if (~kernel::inb(PS2_STATUS) & OUTPUT_ALLOWED)
    {
        return;
    }

    uint8_t code = kernel::inb(0x60);
    if (code < 59)
    {
        lastKey = (!shiftBit ? ScanCodeLookupTable[code] : (ScanCodeLookupTable[code] - 32));
        log::e("k", "C:%c", lastKey);
        keyPressed = true;
    }
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

uint8_t mouseReadByte(void)
{
    waitForOutput();
    return kernel::inb(PS2_DATA);
}

uint8_t getMouseData(void)
{

    waitForOutput();
    return kernel::inb(PS2_DATA);
}

void mouseWriteByte(uint8_t cmd)
{
    waitForInput();
    kernel::outb(PS2_CMD, 0xD4);
    waitForInput();
    kernel::outb(PS2_DATA, cmd);
}

void mouseINIT(void)
{

    bool dualChannel = false;

    kernel::outb(PS2_CMD, 0xAD); // Mask both PS/2 ports
    kernel::outb(PS2_CMD, 0xA7);

    kernel::inb(PS2_DATA); // Flush the PS/2 Buffer

    kernel::outb(PS2_CMD, 0x20);
    waitForOutput();
    uint8_t cc = kernel::inb(PS2_DATA);

    if (cc & (1 << 5))
    {
        dualChannel = true;
    }

    cc = cc & (0b00111111);

    kernel::outb(PS2_CMD, 0x60);
    waitForInput();
    kernel::outb(PS2_DATA, cc); // Set the controller-configuration byte

    kernel::outb(PS2_CMD, 0xAA); // Perform the PS/2 self-test
    waitForOutput();
    if (kernel::inb(PS2_DATA) != 0x55)
    {
        goto testFailed;
    }

    kernel::outb(PS2_CMD, 0xAB);
    waitForOutput();
    if (kernel::inb(PS2_DATA) == 0x00)
    {
        goto testFailed;
    }

    if (dualChannel)
    {
        kernel::outb(PS2_CMD, 0xA9);
        waitForOutput();
        if (kernel::inb(PS2_DATA) != 0x00)
        {
            goto testFailed;
        }
        kernel::outb(PS2_CMD, 0xA8);
    }

    kernel::outb(PS2_CMD, 0xAE);

    kernel::outb(PS2_CMD, 0x20);
    waitForOutput();
    cc = kernel::inb(PS2_DATA);

    cc = cc | 0b1;

    kernel::outb(PS2_CMD, 0x60);
    waitForInput();
    kernel::outb(PS2_DATA, cc); // Set the controller-configuration byte

    mouseWriteByte(0xF6);
    mouseReadByte();

    mouseWriteByte(0xF4);
    mouseReadByte();

    return;

testFailed:

    log::e("PS/2 Driver", "Error initializing PS/2 port!\n");
    return;
}

enum MOUSE_FLAGS
{
    Y_OVERFLOW = (1 << 7),
    X_OVERFLOW = (1 << 6),
    Y_SIGN = (1 << 5),
    X_SIGN = (1 << 4),
    MIDDLE_BTN = (1 << 2),
    RIGHT_BTN = (1 << 1),
    LEFT_BTN = (1 << 0)
};

static int64_t mouse_X = 0, mouse_Y = 0;

void processMousePacket(void)
{
    while (!(kernel::inb(PS2_STATUS) & (1 << 5)))
        ;

    int16_t bytes[3];
    for (int i = 0; i < 3; i++)
    {
        bytes[i] = getMouseData();
    }

    bool xNegative = bytes[0] & X_SIGN;
    bool yNegative = bytes[0] & Y_SIGN;

    if (bytes[0] & Y_OVERFLOW || bytes[0] & X_OVERFLOW)
        return;

    if (!xNegative)
    {
        mouse_X += bytes[1];
    }
    else
    {
        bytes[1] = 256 - bytes[1];
        mouse_X -= bytes[1];
    }

    if (!yNegative)
    {
        mouse_Y -= bytes[1];
    }
    else
    {
        bytes[2] = 256 - bytes[2];
        mouse_Y += bytes[2];
    }

    if (mouse_X < 0)
        mouse_X = 0;
    if (mouse_X > vga::fbuf_info->width - 8)
        mouse_X = vga::fbuf_info->width - 8;

    if (mouse_Y < 0)
        mouse_Y = 0;
    if (mouse_Y > vga::fbuf_info->height - 8)
        mouse_Y = vga::fbuf_info->height - 8;

    // VGA::drawMouse(mouse_X, mouse_Y); UNIMPLEMENTED
}