#include "drivers/vga/vga.h"
#include "kernel/mem/mem.h"
#include "kernel/mem/paging.h"
#include "types.h"
#include <drivers/vga/fonts.h>
#include <stdlib/string.h>
#include <kernel/vfs/file.h>

namespace vga
{

    PSF_header_t *font_hdr;
    uint8_t *bitmap;
    const char *font_path = "/bin/font.psf";

    vga::Color fg;
    vga::Color bg;

    void initialize_font()
    {
        stdlib::string path{font_path};
        kernel::File *font_file = new kernel::File;
        kernel::open(font_file, &path);
        uint8_t *buffer = (uint8_t *)kernel::read(font_file);

        font_hdr = (PSF_header_t *)buffer;
        bitmap = buffer + font_hdr->header_sz;

        set_foreground({255, 255, 255});
        set_background({0, 0, 0});
    }

    void set_foreground(vga::Color new_fg)
    {
        fg = new_fg;
    }

    void set_background(vga::Color new_bg)
    {
        bg = new_bg;
    }

    void putchar(int start_x, int start_y, char ascii)
    {
        uint8_t *glyph = (ascii * font_hdr->glyph_size) + bitmap;

        for (int y = start_y; y < font_hdr->height + start_y; y++)
        {
            for (int x = start_x; x < font_hdr->width + start_x; x++)
            {
                if (*glyph & (1 << (font_hdr->width - x + start_x)))
                {
                    vga::putpixel(x, y, fg);
                }
                else
                {
                    vga::putpixel(x, y, bg);
                }
            }

            glyph += font_hdr->width / 8 + 1;
            if (font_hdr->width % 8 == 0)
            {
                glyph--;
            }
        }
    }

    extern int terminal_x, terminal_y;

    void putstring(const char *data, int x, int y)
    {
        while (*data != '\0')
        {
            if (x >= vga::fbuf_info->width || *data == '\n')
            {
                y += vga::font_hdr->height;
                x = 0;
            }

            if (*data == '\n')
            {
                data++;
                continue;
            }

            putchar(x, y, *data);
            x += font_hdr->width;
            data++;
        }
    }

    void draw_cursor()
    {
        vga::fillRect(terminal_x, terminal_y, {255, 255, 255}, font_hdr->width, font_hdr->height);
    }

    void putstring(const char *data)
    {
        while (*data != '\0')
        {
            if (terminal_x >= vga::fbuf_info->width || *data == '\n')
            {
                vga::fillRect(terminal_x, terminal_y, {0, 0, 0}, font_hdr->width, font_hdr->height);
                terminal_y += vga::font_hdr->height;
                terminal_x = 0;

                if (terminal_y >= vga::fbuf_info->height)
                {
                    vga::scroll_up();
                }
            }

            if (*data == '\n')
            {
                data++;
                continue;
            }

            if (*data == '\t')
            {
                terminal_x += (font_hdr->width * 4) - (terminal_x % (font_hdr->width * 4));
                data++;
                continue;
            }

            if (*data == '\b')
            {
                if (terminal_x > 0)
                {
                    vga::fillRect(terminal_x, terminal_y, {0, 0, 0}, font_hdr->width, font_hdr->height);
                    vga::putchar(terminal_x - font_hdr->width, terminal_y, ' ');
                    terminal_x -= font_hdr->width;
                }

                data++;
                continue;
            }

            putchar(terminal_x, terminal_y, *data);
            terminal_x += font_hdr->width;
            data++;
        }

        draw_cursor();
    }

    extern uint32_t *g_framebuffer2;
    extern uint64_t framebufferSize;

    void scroll_up()
    {
        uint64_t size_of_row = (fbuf_info->pitch * font_hdr->height) / 4;
        for (int i = 0; i < framebufferSize - size_of_row; i++)
        {
            g_framebuffer2[i] = g_framebuffer2[i + size_of_row];
        }

        fillRect(0, fbuf_info->height - font_hdr->height, {0, 0, 0}, fbuf_info->width, font_hdr->height);
    }
}