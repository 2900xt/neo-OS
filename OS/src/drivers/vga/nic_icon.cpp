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

void drawImage(const nic_image *img, int x, int y)
{
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

static const char * NIC_SIGNATURE = "NIC-1.0";


nic_image *loadImage(const char *filepath)
{
    VFS::File *fp = VFS::open(filepath);
    if(fp == NULL)
    {
        return NULL;
    }

    int pages = fp->file_size / 0x1000 + 1;
    nic_image *buffer = (nic_image*)kernel::allocate_pages(pages);
    kernel::map_pages((uint64_t)buffer, (uint64_t)buffer, pages);
    VFS::read(fp, buffer);

    if(!std::strcmp(buffer->signature, NIC_SIGNATURE, 7))
    {
        Log.e(nic_driver_tag, "Invalid NIC File Signature: %ss", buffer);
        return NULL;
    }

    return buffer;
}

void closeImage(nic_image *img)
{
    kernel::free_pages(img);
}

}