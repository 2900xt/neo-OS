#include "kernel/mem/mem.h"
#include "kernel/mem/paging.h"
#include <limine/limine.h>

#include <drivers/vga/vga.h>
#include <kernel/x64/intr/apic.h>
#include <kernel/mem/mem.h>
#include <kernel/vfs/file.h>
#include <kernel/io/log.h>


namespace vga
{

    volatile limine::limine_framebuffer_request fbuf_req = {LIMINE_FRAMEBUFFER_REQUEST, 0, NULL};

    uint32_t *g_framebuffer1;
    uint32_t *g_framebuffer2;
    bool g_framebuffer_dirty = false;

    uint64_t framebufferSize;

    // Map the unused buffer to the address given by limine
    // Unmap the first buffer
    void repaintScreen()
    {
        kernel::memcpy(g_framebuffer1, g_framebuffer2, framebufferSize);
        g_framebuffer_dirty = false;
    }

    void clearScreen()
    {
        kernel::memset(g_framebuffer2, framebufferSize, 0);
        g_framebuffer_dirty = true;
    }

    limine::limine_framebuffer *fbuf_info;

    void putpixel(int x, int y, Color c)
    {
        uint32_t where = x + y * fbuf_info->pitch / 4;
        g_framebuffer2[where] = c.getRGB();
        g_framebuffer_dirty = true;
    }

    void fillRect(int x, int y, Color c, uint32_t w, uint32_t h)
    {
        uint32_t where;
        for (int i = y; i < y + h; i++)
        {
            where = x + i * fbuf_info->pitch / 4;
            for (int j = x; j < x + w; j++)
            {
                g_framebuffer2[where] = c.getRGB();
                where++;
            }
        }
        g_framebuffer_dirty = true;
    }

    void framebuffer_init(void)
    {
        fbuf_info = fbuf_req.response->framebuffers[0];
        framebufferSize = fbuf_info->width + fbuf_info->height * fbuf_info->pitch;
        log.v("Framebuffer", "Width: %d, Height: %d, Pitch: %d", fbuf_info->width, fbuf_info->height, fbuf_info->pitch);
        g_framebuffer1 = (uint32_t *)fbuf_info->address;
        g_framebuffer2 = (uint32_t *)kernel::allocate_pages(framebufferSize / 0x1000 + 1);

        kernel::map_pages((uint64_t)g_framebuffer2, (uint64_t)g_framebuffer2, framebufferSize / 0x1000 + 1);
        g_framebuffer_dirty = true;
    }

}