#include <limine/limine.h>
#include <stdlib/stdlib.h>
#include <drivers/vga/vga.h>
#include <kernel/x64/intr/apic.h>
#include <kernel/mem/mem.h>

volatile limine::limine_framebuffer_request fbuf_req = {LIMINE_FRAMEBUFFER_REQUEST, 0};

uint8_t *g_framebuffer;

limine::limine_framebuffer *fbuf_info;

Color bg;

void setBackgroundColor(Color c)
{
    bg.r = c.r;
    bg.g = c.g;
    bg.b = c.b;
}

Color* getBackgroundColor(void)
{
    return &bg;
}

void putpixel(int x, int y, Color c)
{
    uint32_t where = x * fbuf_info->bpp / 8 + y * fbuf_info->pitch;
    g_framebuffer[where] = c.b;
    g_framebuffer[where + 1] = c.g;
    g_framebuffer[where + 2] = c.r;
}

void fillRect(int x, int y, Color c, uint32_t w, uint32_t h)
{
    uint32_t where;

    for (int i = y; i < h + y; i++)
    {
        where = fbuf_info->pitch * i;
        for (int j = x; j < w + x; j++)
        {
            g_framebuffer[where] = c.b;
            g_framebuffer[where + 1] = c.g;
            g_framebuffer[where + 2] = c.r;

            where += 4;
        }
    }
}

void fbuf_init(void)
{
    fbuf_info = fbuf_req.response->framebuffers[0];
    g_framebuffer = (uint8_t *)fbuf_info->address;
    std::klogf("Found framebuffer at [0x%x]\nWidth = %d\nHeight = %d\n\n",  g_framebuffer, fbuf_info->width, fbuf_info->height);
    setBackgroundColor(Color(0, 0, 0));
}
