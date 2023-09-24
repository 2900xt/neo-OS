#pragma once
#include <types.h>
#include <drivers/vga/vga.h>

#define PSF_FONT_MAGIC 0x864AB572

namespace VGA
{

struct PSF_header_t
{
    uint32_t magic;
    uint32_t version;
    uint32_t header_sz;
    uint32_t flags;
    uint32_t glyph_count;
    uint32_t glyph_size;
    uint32_t height;
    uint32_t width;
}__attribute__((packed));


void initialize_font();
void putstring(const char* data);
void putchar(int x, int y, char ascii);
void set_background(VGA::Color new_bg);
void set_foreground(VGA::Color new_fg);
void scroll_up();

}