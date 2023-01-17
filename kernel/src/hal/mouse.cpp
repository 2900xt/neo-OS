#include <drivers/mouse.h>
#include <arch/amd64/io.h>
#include <stdout.h>
#include <vga/vga.h>

enum PS2_REGS
{
    PS2_DATA    = 0x60,
    PS2_STATUS  = 0x64,
    PS2_CMD     = 0x64,
};

enum PS2_STATUS_FLAGS
{
    OUTPUT_ALLOWED  = (1 << 0),
    INPUT_ALLOWED   = (1 << 1),
    CONTROLLER_DATA = (1 << 3), 
};

void waitForInput(void)
{
    while(!(~inb(PS2_STATUS) & INPUT_ALLOWED));
}

void waitForOutput(void)
{
    while(!(inb(PS2_STATUS) & OUTPUT_ALLOWED));
}

uint8_t mouseReadByte(void)
{
    waitForOutput();
    return inb(PS2_DATA);
}

uint8_t getMouseData(void)
{
    
    waitForOutput();
    return inb(PS2_DATA);
}

void mouseWriteByte(uint8_t cmd)
{
    waitForInput();
    outb(PS2_CMD, 0xD4);
    waitForInput();
    outb(PS2_DATA, cmd);
}


void mouseINIT(void)
{

    bool dualChannel = false;

    outb(PS2_CMD, 0xAD);        //Mask both PS/2 ports
    outb(PS2_CMD, 0xA7);

    inb(PS2_DATA);              //Flush the PS/2 Buffer

    outb(PS2_CMD, 0x20);
    waitForOutput();
    uint8_t cc = inb(PS2_DATA);

    if(cc & (1 << 5))
    {
        dualChannel = true;
    }

    cc = cc & (0b00111111);

    outb(PS2_CMD, 0x60);
    waitForInput();
    outb(PS2_DATA, cc);         //Set the controller-configuration byte


    outb(PS2_CMD, 0xAA);        //Perform the PS/2 self-test
    waitForOutput();
    if(inb(PS2_DATA) != 0x55)
    {
        goto testFailed;
    }

    outb(PS2_CMD, 0xAB);
    waitForOutput();
    if(inb(PS2_DATA) != 0x00)
    {
        goto testFailed;
    }

    if(dualChannel)
    {
        outb(PS2_CMD, 0xA9);
        waitForOutput();
        if(inb(PS2_DATA) != 0x00)
        {
            goto testFailed;
        }
        outb(PS2_CMD, 0xA8);
    }

    outb(PS2_CMD, 0xAE);

    outb(PS2_CMD, 0x20);
    waitForOutput();
    cc = inb(PS2_DATA);

    cc = cc | 0b1;

    outb(PS2_CMD, 0x60);
    waitForInput();
    outb(PS2_DATA, cc);         //Set the controller-configuration byte

    mouseWriteByte(0xF6);
    mouseReadByte();

    mouseWriteByte(0xF4);
    mouseReadByte();

    return;

testFailed:

    neoSTL::printf("Error initializing PS/2 port!\n");
    return;

}

enum MOUSE_FLAGS
{
    Y_OVERFLOW      = (1 << 7),
    X_OVERFLOW      = (1 << 6),
    Y_SIGN          = (1 << 5),
    X_SIGN          = (1 << 4),
    MIDDLE_BTN      = (1 << 2),
    RIGHT_BTN       = (1 << 1),
    LEFT_BTN        = (1 << 0)
};


static int64_t mouse_X = 0, mouse_Y = 0;

void processMousePacket(void)
{
    while(!(inb(PS2_STATUS) & (1 << 5)));

    int16_t bytes[3];
    for(int i = 0; i < 3; i++)
    {
        bytes[i] = getMouseData();
    }

    bool xNegative = bytes[0] & X_SIGN;
    bool yNegative = bytes[0] & Y_SIGN;

    if(bytes[0] & Y_OVERFLOW || bytes[0] & X_OVERFLOW) return;

    if(!xNegative){
        mouse_X += bytes[1];
    } else {
        bytes[1] = 256 - bytes[1];
        mouse_X -= bytes[1];
    }

    if(!yNegative){
        mouse_Y -= bytes[1];
    } else {
        bytes[2] = 256 - bytes[2];
        mouse_Y += bytes[2];
    }

    if(mouse_X < 0) mouse_X = 0;
    if(mouse_X > fbuf_info->width - 8) mouse_X = fbuf_info->width - 8;


    if(mouse_Y < 0) mouse_Y = 0;
    if(mouse_Y > fbuf_info->height - 8) mouse_Y = fbuf_info->height - 8;

    drawMouse(mouse_X, mouse_Y);

}