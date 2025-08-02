#include <drivers/fs/fat/fat.h>
#include <drivers/vga/fonts.h>
#include <drivers/vga/vga.h>
#include <kernel/kernel.h>

// External variable declarations for system info
extern uint64_t millis_since_boot;

namespace vga {
extern PSF_header_t *font_hdr;
extern limine::limine_framebuffer *fbuf_info;
} // namespace vga

// External declarations for system information
extern volatile limine::limine_smp_request smp_request;
extern uint64_t heapBlkcount;
extern uint64_t heapBlksize;

namespace kernel {
void display_fetch() {
  // Display neo-OS ASCII art and system info
  terminal_puts("\n");

  // Simple ASCII art for neo-OS
  kernel::printf("%p      ___  ___         %p\n",
                 vga::Color(0, 255, 255).getRGB(),
                 vga::Color(255, 255, 255).getRGB());
  kernel::printf("%p     |   \\/   |        %p\n",
                 vga::Color(0, 255, 255).getRGB(),
                 vga::Color(255, 255, 255).getRGB());
  kernel::printf("%p     | |\\/| | ___     %p\n",
                 vga::Color(0, 255, 255).getRGB(),
                 vga::Color(255, 255, 255).getRGB());
  kernel::printf("%p     | |  | |/ _ \\    %p\n",
                 vga::Color(0, 255, 255).getRGB(),
                 vga::Color(255, 255, 255).getRGB());
  kernel::printf("%p     | |  | |  __/    %p\n",
                 vga::Color(0, 255, 255).getRGB(),
                 vga::Color(255, 255, 255).getRGB());
  kernel::printf("%p     |_|  |_|\\___|    %p\n",
                 vga::Color(0, 255, 255).getRGB(),
                 vga::Color(255, 255, 255).getRGB());
  kernel::printf("%p        neo-OS        %p\n",
                 vga::Color(0, 255, 255).getRGB(),
                 vga::Color(255, 255, 255).getRGB());
  terminal_puts("\n");

  // System information
  kernel::printf("%pOS:%p neo-OS x86_64\n", vga::Color(255, 100, 100).getRGB(),
                 vga::Color(255, 255, 255).getRGB());

  kernel::printf("%pKernel:%p neo-OS kernel\n",
                 vga::Color(255, 100, 100).getRGB(),
                 vga::Color(255, 255, 255).getRGB());

  // CPU info
  if (smp_request.response != nullptr) {
    kernel::printf("%pCPU:%p %u cores\n", vga::Color(255, 100, 100).getRGB(),
                   vga::Color(255, 255, 255).getRGB(),
                   smp_request.response->cpu_count);
  } else {
    kernel::printf("%pCPU:%p Unknown\n", vga::Color(255, 100, 100).getRGB(),
                   vga::Color(255, 255, 255).getRGB());
  }

  // Memory info
  uint64_t total_heap_mb = (heapBlkcount * heapBlksize) / (1024 * 1024);
  kernel::printf("%pMemory:%p %u MB heap allocated\n",
                 vga::Color(255, 100, 100).getRGB(),
                 vga::Color(255, 255, 255).getRGB(), total_heap_mb);

  // Display resolution
  if (vga::fbuf_info != nullptr) {
    kernel::printf("%pResolution:%p %ux%u\n",
                   vga::Color(255, 100, 100).getRGB(),
                   vga::Color(255, 255, 255).getRGB(), vga::fbuf_info->width,
                   vga::fbuf_info->height);
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
  kernel::printf("%p█%p", vga::Color(255, 0, 0).getRGB(),
                 vga::Color(255, 255, 255).getRGB());
  kernel::printf("%p█%p", vga::Color(0, 255, 0).getRGB(),
                 vga::Color(255, 255, 255).getRGB());
  kernel::printf("%p█%p", vga::Color(0, 0, 255).getRGB(),
                 vga::Color(255, 255, 255).getRGB());
  kernel::printf("%p█%p", vga::Color(255, 255, 0).getRGB(),
                 vga::Color(255, 255, 255).getRGB());
  kernel::printf("%p█%p", vga::Color(255, 0, 255).getRGB(),
                 vga::Color(255, 255, 255).getRGB());
  kernel::printf("%p█%p", vga::Color(0, 255, 255).getRGB(),
                 vga::Color(255, 255, 255).getRGB());
  terminal_puts("\n\n");
}
} // namespace kernel