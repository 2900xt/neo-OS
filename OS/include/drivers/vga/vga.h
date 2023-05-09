#ifndef VGA_VGA_H
#define VGA_VGA_H

#include <limine/limine.h>
#include <types.h>

extern limine::limine_framebuffer *fbuf_info;

class Color
{
public:
    Color(uint8_t r, uint8_t g, uint8_t b)
    {
        rgb = (b << fbuf_info->blue_mask_shift) | (g << fbuf_info->green_mask_shift) | (r << fbuf_info->red_mask_shift);
    }

    Color(Color const &other)
    {
        rgb = other.rgb;
    }

    Color()
    {
        rgb = 0;
    }

    uint32_t getRGB()
    {
        return rgb;
    }

    Color const &operator=(Color const &other)
    {
        this->rgb = other.rgb;
        return *this;
    }

protected:
    uint32_t rgb;
};

struct image 
{
    uint32_t x, y, w, h;
    uint32_t *data;
};

extern limine::limine_framebuffer *fbuf_info;

void drawMouse(uint64_t x, uint64_t y);
void setBackgroundColor(Color c);
Color* getBackgroundColor(void);
void fbuf_init(void);
void repaintScreen();
void fillRect(int x, int y, Color c, uint32_t w, uint32_t h);
image *tga_parse(void *ptr, int size);

#endif