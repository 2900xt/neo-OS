#include "kernel/mem/paging.h"
#include "kernel/vfs/file.h"
#include "stdlib/stdio.h"
#include "stdlib/string.h"
#include <types.h>
#include <kernel/mem/mem.h>
#include <drivers/vga/vga.h>    

namespace VGA {

extern uint32_t *g_framebuffer2;

static const char * nic_driver_tag = "NIC Image Driver";
static const char * NIC_SIGNATURE = "NIC-1.0";

void drawImage(nic_image *img, int x, int y)
{
     if(!std::strcmp(img->signature, NIC_SIGNATURE, 7))
    {
        Log.e(nic_driver_tag, "Invalid NIC File Signature: %s", img->signature);
        return;
    }

    uint32_t where;
    uint32_t ptr = 0;
    for (int i =  y; i < y + img->h; i++)
    {
        where = x + i * fbuf_info->pitch / 4;
        for (int j = x; j < x + img->w; j++)
        {
            int rgba = img->data[ptr];

            if(rgba & 0xFF)
            {
                g_framebuffer2[where] = rgba;
            }
            
            ptr++;
            where++;
        }
    }
}


void closeImage(nic_image *img)
{
    kernel::free_pages(img);
}

}