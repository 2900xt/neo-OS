#ifndef VGA_VGA_H
#define VGA_VGA_H

#include <limine/limine.h>
#include <types.h>

namespace VGA {
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

struct nic_image 
{
    char signature[7];
    uint32_t w, h;
    uint32_t data[1];
}__attribute__((packed));

extern limine::limine_framebuffer *fbuf_info;

void drawMouse(uint64_t x, uint64_t y);
void setBackgroundColor(Color c);
Color* getBackgroundColor(void);
void fbuf_init(void);
void repaintScreen();
void clearScreen();

void fillRect(int x, int y, Color c, uint32_t w, uint32_t h);
void putpixel(int x, int y, Color c);
void drawImage(nic_image *img, int x, int y);
nic_image *loadImage(const char *filepath);

}

#endif