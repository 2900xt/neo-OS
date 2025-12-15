#include "kernel/shell/shell.h"
#include <drivers/fs/fat/fat.h>
#include <drivers/vga/fonts.h>
#include <drivers/vga/vga.h>


extern uint64_t millis_since_boot;

namespace vga
{
  extern PSF_header_t *font_hdr;
  extern limine::limine_framebuffer *fbuf_info;
} // namespace vga

namespace kernel
{
  // External declarations for system information
  extern volatile limine::limine_smp_request smp_request;
  extern uint64_t heapBlkcount;
  extern uint64_t heapBlksize;

  void display_fetch()
  {
    // Display neo-OS ASCII art and system info
    terminal_puts("\n");

    // Enhanced ASCII art logo for neo-OS (ASCII only)
    kernel::printf("%p    ####   ##  ######   #######           #######   ######    %p\n",
                   vga::Color(0, 255, 255).getRGB(),
                   vga::Color(255, 255, 255).getRGB());
    kernel::printf("%p    ## ##  ##  ##       ##   ##  ######   ##   ##  ##         %p\n",
                   vga::Color(0, 220, 255).getRGB(),
                   vga::Color(255, 255, 255).getRGB());
    kernel::printf("%p    ##  ## ##  ####     ##   ##           ##   ##   #####     %p\n",
                   vga::Color(0, 180, 255).getRGB(),
                   vga::Color(255, 255, 255).getRGB());
    kernel::printf("%p    ##   ####  ##       ##   ##           ##   ##       ##    %p\n",
                   vga::Color(0, 140, 255).getRGB(),
                   vga::Color(255, 255, 255).getRGB());
    kernel::printf("%p    ##    ###  ######   #######           #######   ######    %p\n",
                   vga::Color(0, 100, 255).getRGB(),
                   vga::Color(255, 255, 255).getRGB());
    kernel::printf("%p                                                               %p\n",
                   vga::Color(0, 80, 255).getRGB(),
                   vga::Color(255, 255, 255).getRGB());
    kernel::printf("%p           -= Next Generation Operating System =-            %p\n",
                   vga::Color(100, 150, 255).getRGB(),
                   vga::Color(255, 255, 255).getRGB());
    terminal_puts("\n");

    // System information
    kernel::printf("%pOS:%p neo-OS x86_64\n", vga::Color(255, 100, 100).getRGB(),
                   vga::Color(255, 255, 255).getRGB());

    kernel::printf("%pKernel:%p neo-OS kernel\n",
                   vga::Color(255, 100, 100).getRGB(),
                   vga::Color(255, 255, 255).getRGB());

    // CPU info
    if (smp_request.response != NULL)
    {
      kernel::printf("%pCPU:%p %u cores\n", vga::Color(255, 100, 100).getRGB(),
                     vga::Color(255, 255, 255).getRGB(),
                     smp_request.response->cpu_count);
    }
    else
    {
      kernel::printf("%pCPU:%p Unknown\n", vga::Color(255, 100, 100).getRGB(),
                     vga::Color(255, 255, 255).getRGB());
    }

    // Memory info
    uint64_t total_heap_mb = (heapBlkcount * heapBlksize) / (1024 * 1024);
    kernel::printf("%pMemory:%p %u MB heap allocated\n",
                   vga::Color(255, 100, 100).getRGB(),
                   vga::Color(255, 255, 255).getRGB(), total_heap_mb);
    // Display resolution
    if (vga::fbuf_info != NULL)
    {
      kernel::printf("%pResolution:%p %ux%u\n",
                     vga::Color(255, 100, 100).getRGB(),
                     vga::Color(255, 255, 255).getRGB(), vga::fbuf_info->width,
                     vga::fbuf_info->height);
    }

    // Font information
    if (vga::font_hdr != NULL)
    {
      kernel::printf("%pFont:%p PSF %ux%u (%u glyphs, %u bytes/glyph)\n",
                     vga::Color(255, 100, 100).getRGB(),
                     vga::Color(255, 255, 255).getRGB(),
                     vga::font_hdr->width,
                     vga::font_hdr->height,
                     vga::font_hdr->glyph_count,
                     vga::font_hdr->glyph_size);
    }
    else
    {
      kernel::printf("%pFont:%p Unknown\n",
                     vga::Color(255, 100, 100).getRGB(),
                     vga::Color(255, 255, 255).getRGB());
    }

    // Uptime
    uint64_t uptime_seconds = millis_since_boot / 1000;
    uint64_t hours = uptime_seconds / 3600;
    uint64_t minutes = (uptime_seconds % 3600) / 60;
    uint64_t seconds = uptime_seconds % 60;

    kernel::printf("%pUptime:%p %uh %um %us\n",
                   vga::Color(255, 100, 100).getRGB(),
                   vga::Color(255, 255, 255).getRGB(), hours, minutes, seconds);

    // Color blocks for terminal colors demonstration
    kernel::printf("%pColors:%p ", vga::Color(255, 100, 100).getRGB(),
                   vga::Color(255, 255, 255).getRGB());
    kernel::printf("%p%a%p", vga::Color(255, 0, 0).getRGB(),
                   vga::Color(255, 255, 255).getRGB());
    kernel::printf("%p%a%p", vga::Color(0, 255, 0).getRGB(),
                   vga::Color(255, 255, 255).getRGB());
    kernel::printf("%p%a%p", vga::Color(0, 0, 255).getRGB(),
                   vga::Color(255, 255, 255).getRGB());
    kernel::printf("%p%a%p", vga::Color(255, 255, 0).getRGB(),
                   vga::Color(255, 255, 255).getRGB());
    kernel::printf("%p%a%p", vga::Color(255, 0, 255).getRGB(),
                   vga::Color(255, 255, 255).getRGB());
    kernel::printf("%p%a%p", vga::Color(0, 255, 255).getRGB(),
                   vga::Color(255, 255, 255).getRGB());
    terminal_puts("\n\n");
  }
} // namespace kernel