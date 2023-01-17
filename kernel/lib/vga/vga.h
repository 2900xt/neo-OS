#ifndef VGA_VGA_H
#define VGA_VGA_H

class Color
{
public:
    uint8_t r, g, b;

    Color()
    {
        r = 0;
        g = 0;
        b = 0;
    }

    Color(uint8_t r, uint8_t g, uint8_t b)
    {
        this->r = r;
        this->g = g;
        this->b = b;
    }
};

extern limine::limine_framebuffer *fbuf_info;

void drawMouse(uint64_t x, uint64_t y);

void fbuf_init(void);

#endif