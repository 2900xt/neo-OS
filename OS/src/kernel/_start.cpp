#include <limine/limine.h>
#include <kernel/io/log.h>
#include <kernel/x64/io.h>
#include <kernel/x64/intr/idt.h>
#include <kernel/mem/mem.h>
#include <kernel/mem/paging.h>
#include <drivers/vga/vga.h>
#include <drivers/pci/pci.h>
#include <drivers/disk/ahci/ahci.h>
#include <drivers/vga/fonts.h>
#include <drivers/network/rtl8139.h>
#include <kernel/vfs/file.h>
#include <kernel/smp/smp.h>
#include <kernel/shell/shell.h>
#include <kernel/x64/intr/apic.h>
#include <stdlib/timer.h>
#include <kernel/io/scan.h>
#include <kernel/io/log.h>


namespace kernel
{
    const char *kernel_tag = "Kernel";



    extern "C" void _start(void)
    {
        kernel::log_init();
        kernel::enable_sse();
        kernel::fill_idt();
        kernel::heapInit();
        kernel::initialize_page_allocator();
        vga::framebuffer_init();

        pci::enumerate_pci();
        disk::ahci_init();
        kernel::vfs_init();
        vga::initialize_font();
        network::rtl8139_init();
        kernel::smp_init();
        
        wm::window_manager_init();
        log::d("Startup", "Window manager initialized, creating terminal window...");
        
        // Debug: Check font status right before window creation
        log::d("Startup", "Verifying font before window creation: font_hdr=%x", (uint64_t)vga::font_hdr);
        if (vga::font_hdr) {
            log::d("Startup", "Font details: width=%d, height=%d, magic=%x", 
                   vga::font_hdr->width, vga::font_hdr->height, vga::font_hdr->magic);
        } else {
            log::e("Startup", "Font header is null!");
        }
        
        // Create initial login window
        wm::Window* login_window = wm::create_login_window(200, 150, 400, 200);
        if (login_window)
        {
            log::d("Startup", "Login window created successfully");
            wm::set_window_focus(login_window);
        }
        else
        {
            log::e("Startup", "Failed to create login window!");
        }

        static int main_loop_count = 0;
        static uint64_t last_clock_update = 0;
        static uint64_t last_snake_update = 0;
        while (true)
        {
            kernel::sleep(5);
            stdlib::call_timers();
            
            // Update clock windows every 1000ms (1 second)
            if (millis_since_boot - last_clock_update >= 1000)
            {
                // Update all clock windows
                for (int i = 0; i < wm::window_count; i++)
                {
                    if (wm::windows[i] && wm::windows[i]->type == wm::WINDOW_TYPE_CLOCK)
                    {
                        wm::update_clock_display(wm::windows[i]);
                    }
                }
                last_clock_update = millis_since_boot;
            }
            
            // Update snake games every 200ms
            if (millis_since_boot - last_snake_update >= 200)
            {
                for (int i = 0; i < wm::window_count; i++)
                {
                    if (wm::windows[i] && wm::windows[i]->type == wm::WINDOW_TYPE_SNAKE)
                    {
                        wm::update_snake_game(wm::windows[i]);
                        wm::render_snake_game(wm::windows[i]);
                    }
                }
                last_snake_update = millis_since_boot;
            }
            
            // Debug: Show main loop status
            if (main_loop_count < 3)
            {
                //log::d("MainLoop", "Main loop iteration %d", main_loop_count);
                main_loop_count++;
            }
            
            // Handle keyboard input for window management
            if (ps2::pollKeyInput())
            {
                // Check for special keys (arrow keys, function keys)
                if (ps2::lastSpecialKey > 0)
                {
                    wm::handle_window_special_key(ps2::lastSpecialKey);
                    ps2::lastSpecialKey = 0; // Reset special key
                }
                else if (ps2::lastKey > 0)
                {
                    // Check for Ctrl modifier
                    bool ctrl_pressed = false; // TODO: implement proper Ctrl detection
                    wm::handle_window_keyboard_input(ps2::lastKey, ps2::shiftBit, ctrl_pressed, false);
                }
            }

            // Handle mouse input
            /*if (ps2::pollMouseInput())
            {
                wm::handle_window_mouse_input(ps2::mouse_state.x, ps2::mouse_state.y, 
                                            ps2::mouse_state.left_button, ps2::mouse_state.right_button);
            }*/

            // Render all windows
            wm::render_all_windows();
            
            if (vga::g_framebuffer_dirty)
            {
                //if (main_loop_count <= 3) log::d("MainLoop", "Repainting screen (dirty flag set)");
                vga::repaintScreen();
            }
            else if (main_loop_count <= 3)
            {
                //log::d("MainLoop", "Screen not dirty, skipping repaint");
            }
            
            asm volatile("hlt");
        }

        __builtin_unreachable();
    }
}