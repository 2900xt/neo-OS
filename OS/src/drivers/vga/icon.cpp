#include "kernel/mem/paging.h"
#include "kernel/vfs/file.h"
#include "stdlib/stdio.h"
#include "stdlib/string.h"
#include <types.h>
#include <kernel/mem/mem.h>
#include <drivers/vga/vga.h>    

extern uint32_t *g_framebuffer2;

void drawImage(const image *img, int x, int y)
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

const char * NIC_SIGNATURE = "NIC-1.0";

image *loadImage(const char *filepath)
{
    VFS::File *fp = VFS::open(filepath);
    if(fp == NULL)
    {
        return NULL;
    }

    int pages = fp->file_size / 0x1000 + 1;
    image *buffer = (image*)kernel::allocate_pages(pages);
    kernel::map_pages((uint64_t)buffer, (uint64_t)buffer, pages);
    VFS::read(fp, buffer);

    if(!std::strcmp(buffer->signature, NIC_SIGNATURE, 7))
    {
        std::klogf("Invalid NIC File Signature: %s\n", buffer);
        return NULL;
    }

    return buffer;
}

void closeImage(image *img)
{
    kernel::free_pages(img);
}