#include "kernel/mem/mem.h"
#include "kernel/mem/paging.h"
#include "stdlib/stdio.h"
#include "types.h"
#include <limine/limine.h>
#include <stdlib/stdlib.h>
#include <drivers/vga/vga.h>
#include <kernel/x64/intr/apic.h>
#include <kernel/mem/mem.h>

volatile limine::limine_framebuffer_request fbuf_req = {LIMINE_FRAMEBUFFER_REQUEST, 0};

uint32_t *g_framebuffer1;
uint32_t *g_framebuffer2;

uint64_t framebufferSize;

//Map the unused buffer to the address given by limine
//Unmap the first buffer
void repaintScreen()
{
    memcpy(g_framebuffer1, g_framebuffer2, framebufferSize);
}

limine::limine_framebuffer *fbuf_info;

void putpixel(int x, int y, Color c)
{
    uint32_t where = x * fbuf_info->bpp / 8 + y * fbuf_info->pitch;
    g_framebuffer2[where] = c.getRGB();
}

void fillRect(int x, int y, Color c, uint32_t w, uint32_t h)
{
    uint32_t where;
    for (int i = y; i < h + y; i++)
    {
        where = (fbuf_info->pitch * i) / 4;
        for (int j = x; j < w + x; j++)
        {
            g_framebuffer2[where] = c.getRGB();
            where++;
        }
    }
}

void fbuf_init(void)
{
    fbuf_info = fbuf_req.response->framebuffers[0];
    framebufferSize =  fbuf_info->width * fbuf_info->bpp / 8.0 + fbuf_info->height * fbuf_info->pitch;
    g_framebuffer1 = (uint32_t*)fbuf_info->address;
    g_framebuffer2 = (uint32_t*)kernel::allocate_pages(framebufferSize / 0x1000  + 1);
    kernel::map_pages((uint64_t)g_framebuffer2, (uint64_t)g_framebuffer2, framebufferSize / 0x1000  + 1);
    std::klogf("Sz: 0x%x\n", framebufferSize);
    fillRect(0, 0, {255, 0, 0}, fbuf_info->width, fbuf_info->height);
    repaintScreen();
}
