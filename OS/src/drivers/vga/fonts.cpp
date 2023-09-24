#include "drivers/vga/vga.h"
#include "kernel/mem/mem.h"
#include "kernel/mem/paging.h"
#include "types.h"
#include <drivers/vga/fonts.h>
#include <stdlib/string.h>
#include <kernel/vfs/file.h>

namespace VGA {

PSF_header_t* font_hdr;
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

    font_hdr = (PSF_header_t*)buffer;
    bitmap = buffer + font_hdr->header_sz;

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
    uint8_t* glyph = (ascii * font_hdr->glyph_size) + bitmap;

    for(int y = start_y; y < font_hdr->height + start_y; y++)
    {
        for(int x = start_x; x < font_hdr->width + start_x; x++)
        {
            if(*glyph & (1 << (font_hdr->width - x + start_x)))
            {
                VGA::putpixel(x, y, fg);
            }
            else 
            {
                VGA::putpixel(x, y, bg);
            }
        }

        glyph += font_hdr->width / 8 + 1;
        if(font_hdr->width % 8 == 0)
        {
            glyph--;
        }
    }
}

extern int terminal_x, terminal_y;

void putstring(const char* data)
{
    while(*data != '\0')
    {
        if(terminal_x >= VGA::fbuf_info->width || *data == '\n')
        {
            terminal_y += VGA::font_hdr->height;
            terminal_x = 0;

            if(terminal_y >= VGA::fbuf_info->height)
            {
                VGA::scroll_up();
            }
        }

        if(*data == '\n') 
        {
            data++; continue;
        }

        putchar(terminal_x, terminal_y, *data);
        terminal_x += font_hdr->width;
        data++;
    }
}

extern uint32_t *g_framebuffer2;
extern uint64_t framebufferSize;

void scroll_up()
{
    uint64_t size_of_row = (fbuf_info->pitch * font_hdr->height) / 4;
    for(int i = 0; i < framebufferSize - size_of_row; i++)
    {
        g_framebuffer2[i] = g_framebuffer2[i + size_of_row];
    }

    fillRect(0, fbuf_info->height - font_hdr->height, {0, 0, 0}, fbuf_info->width, font_hdr->height);
}
}