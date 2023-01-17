#include <limine/limine.h>
#include <stdout.h>
#include <vga/vga.h>
#include <vga/gfx.h>
#include <mem.h>
#include <math.h>

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

//WIP

void fillPolygon(int xPoints[], int yPoints[], int n, Color c)
{
    int maxX = 0, maxY = 0, minX = 0, minY = 0;

    Edge* edges = (Edge*)neoSTL::kmalloc(n * sizeof(Edge));

    Edge* activeEdges = (Edge*)neoSTL::kmalloc(n * sizeof(Edge));
    int activeEdgeCount = 0;

    for(int i = 0; i < n - 1; i++)
    {
        maxX = max(xPoints[i], maxX);
        maxY = max(yPoints[i], maxY);
        minX = min(xPoints[i], minX);
        minY = min(yPoints[i], minY);

        edges[i] = Edge(xPoints[i], yPoints[i], xPoints[i + 1], yPoints[i + 1]);
    }

    maxX = max(xPoints[n], maxX);
    maxY = max(yPoints[n], maxY);
    minX = min(xPoints[n], minX);
    minY = min(yPoints[n], minY);

    edges[n] = Edge(xPoints[n], yPoints[n], xPoints[0], yPoints[0]);

    int currentScanline = minY;

    while(currentScanline != maxY);


    neoSTL::kfree(edges);
}

const uint8_t mouse[] = 
{
    0b01000000,
    0b01100000,
    0b01110000,
    0b01111000,
    0b01111100,
    0b01111110,
    0b00000100,
    0b00000010
}; 

void drawMouse(uint64_t x, uint64_t y)
{
    for(int i = y; i < y + 8 ; i++)
    {
        for(int j = x; j < x + 8; j++)
        {
            if(mouse[i - y] & (1 << (j - x)))
            {
                putpixel(j, i, Color(255, 255, 255));
            }
        }
    }
}

void fbuf_init(void)
{
    fbuf_info = fbuf_req.response->framebuffers[0];
    g_framebuffer = (uint8_t *)fbuf_info->address;
    neoSTL::printf("Framebuffer found!\nWidth = %d\nHeight = %d\nPitch = %d\nBPP = %d\nAddr: 0x%x\n", fbuf_info->width, fbuf_info->height, fbuf_info->pitch, fbuf_info->bpp, g_framebuffer);
    
    setBackgroundColor(Color(20, 20, 20));
    fillRect(0, 0, bg, fbuf_info->width, fbuf_info->height);
    drawMouse(10, 10);
}
