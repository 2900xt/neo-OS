#include "kernel/mem/paging.h"
#include "kernel/vfs/file.h"


#include <types.h>
#include <kernel/mem/mem.h>
#include <drivers/vga/vga.h>
#include <kernel/io/log.h>
#include <stdlib/string.h>

namespace vga
{
    extern uint32_t *g_framebuffer2;

    static const char *nic_driver_tag = "NIC Image Driver";
    static const char *NIC_SIGNATURE = "NIC-1.0";

    void drawImage(nic_image *img, int x, int y, int w, int h)
    {
        if (!stdlib::strcmp(img->signature, NIC_SIGNATURE, 7))
        {
            log::e(nic_driver_tag, "Invalid NIC File Signature: %s", img->signature);
            return;
        }

        uint32_t where;
        for (int i = 0; i < h; i++)
        {
            where = x + (i + y) * fbuf_info->pitch / 4;
            for (int j = 0; j < w; j++)
            {
                int I = (i * img->h) / h;
                int J = (j * img->w) / w;
                int rgba = img->data[I * img->w + J];

                if (rgba & 0xFF)
                {
                    g_framebuffer2[where] = rgba;
                }
                where++;
            }
        }
    }

    void closeImage(nic_image *img)
    {
        kernel::free_pages(img);
    }

}