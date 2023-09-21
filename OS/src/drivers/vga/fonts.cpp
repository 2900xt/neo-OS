#include "kernel/mem/paging.h"
#include "types.h"
#include <drivers/vga/fonts.h>
#include <stdlib/string.h>
#include <kernel/vfs/file.h>

namespace VGA {

PSF_header_t* hdr;
uint8_t* bitmap;
const char* font_path = "/bin/font.psf";

VGA::Color fg;
VGA::Color bg;
    
void initialize_font()
{
    std::string path {font_path};
    VFS::File* font_file = new VFS::File;
    VFS::open(font_file, &path);
    uint8_t* buffer = (uint8_t*)VFS::read(font_file);

    hdr = (PSF_header_t*)buffer;
    bitmap = buffer + hdr->header_sz;

    set_foreground({255, 255, 255});
    set_background({0, 0, 0});
}

void set_foreground(VGA::Color new_fg)
{
    fg = new_fg;
}


void set_background(VGA::Color new_bg)
{
    bg = new_bg;
}

void putchar(int start_x, int start_y, char ascii)
{
    uint8_t* glyph = (ascii * hdr->glyph_size) + bitmap;

    for(int y = start_y; y < hdr->height + start_y; y++)
    {
        for(int x = start_x; x < hdr->width + start_x; x++)
        {
            if(*glyph & (1 << (hdr->width - x + start_x)))
            {
                VGA::putpixel(x, y, fg);
            }
            else 
            {
                VGA::putpixel(x, y, bg);
            }
        }

        glyph += hdr->width / 8 + 1;
        if(hdr->width % 8 == 0)
        {
            glyph--;
        }
    }
}

void putstring(int x, int y, const char* data)
{
    while(*data != '\0')
    {
        putchar(x, y, *data);
        x += hdr->width;
        data++;
    }
}
}