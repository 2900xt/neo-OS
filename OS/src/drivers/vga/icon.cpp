#include "kernel/mem/paging.h"
#include "stdlib/stdio.h"
#include <types.h>
#include <kernel/mem/mem.h>
#include <drivers/vga/vga.h>    

/**
 * Source: https://wiki.osdev.org/Loading_Icons 
 *
 * Parse TGA format into pixels. Returns NULL or error, otherwise the returned data looks like
 *   ret[0] = width of the image
 *   ret[1] = height of the image
 *   ret[2..] = 32 bit ARGB pixels (blue channel in the least significant byte, alpha channel in the most)
 */

extern uint32_t *g_framebuffer2;

struct tga_hdr
{
    uint8_t     id_length;
    uint8_t     color_map_type;
    uint8_t     image_type;

    //Color Map Specification
    uint16_t    cmap_first_entry;
    uint16_t    cmap_length;
    uint8_t     cmap_entry_sz;

    //Image Specification
    uint16_t    origin_x;
    uint16_t    origin_y;
    uint16_t    img_width;
    uint16_t    img_height;
    uint8_t     px_depth;

    //Image Descriptor
    uint8_t     attrib_per_pixel : 4;
    uint8_t     pixel_data_order : 2;
    uint8_t     rsvd : 2;
}__attribute__((packed));


image *tga_parse(void *ptr, int size)
{
    tga_hdr *hdr = (tga_hdr*)ptr;
    if(hdr->color_map_type != 0x00 && hdr->image_type != 0x02)
    {
        std::klogf("Unsupported TGA Image Format!\n");
        return NULL;
    }

    if(hdr->px_depth != 32)
    {
        std::klogf("Unsupported TGA Color Map!\n");
        return NULL;
    }

    image *img = new image;

    uint64_t page_count = ((hdr->img_width * hdr->img_height * hdr->px_depth / 8) + 2) / 0x1000 + 1;
    uint32_t *data = (uint32_t*)kernel::allocate_pages(page_count);

    uint32_t data_index = sizeof(tga_hdr) + hdr->id_length + hdr->cmap_length;
    uint32_t* pixels = (uint32_t*)ptr + data_index;

    img->data = pixels;
    img->x = hdr->origin_x;
    img->y = hdr->origin_y;
    img->w = hdr->img_width;
    img->h = hdr->img_height;

    return img;
}

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