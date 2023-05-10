#include "kernel/mem/paging.h"
#include "kernel/vfs/file.h"
#include "stdlib/stdio.h"
#include "stdlib/string.h"
#include <types.h>
#include <kernel/mem/mem.h>
#include <drivers/vga/vga.h>    

extern uint32_t *g_framebuffer2;

void drawImage(const image *img)
{
    uint32_t where;
    uint32_t ptr = 0;
    for (int i = 0; i < img->h; i++)
    {
        where = (fbuf_info->pitch * i) / 4;
        for (int j = 0; j < img->h; j++)
        {
            g_framebuffer2[where] = img->data[ptr++];
            where++;
        }
    }
}

image *loadImage(const char *filepath)
{
    VFS::File *fp = VFS::open(filepath);
    if(fp == NULL)
    {
        return NULL;
    }

    int pages = fp->file_size / 0x1000 + 1;
    char *buffer = (char*)kernel::allocate_pages(pages);
    char *_buffer = buffer;
    kernel::map_pages((uint64_t)buffer, (uint64_t)buffer, pages);
    VFS::read(fp, buffer);

    image *img = new image;

    int n = std::strlen(buffer);
    img->w = std::atou(buffer, n);
    buffer += n + 1;

    n = std::strlen(buffer);
    img->h = std::atou(buffer, n);
    buffer += n + 1;

    pages = (img->w * img->h * sizeof(uint32_t)) / 0x1000 + 1;
    img->data = (uint32_t*)kernel::allocate_pages(pages);
    kernel::map_pages((uint64_t)img->data, (uint64_t)img->data, pages);

    for(int i = 0; i < img->w * img->h; i++)
    {
        n = std::strlen(buffer);
        img->data[i] = std::atou(buffer, n);
        buffer += n + 1;
    }

    kernel::free_pages(_buffer);
    return img;
}