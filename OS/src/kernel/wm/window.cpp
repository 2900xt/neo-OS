#include <kernel/wm/window.h>
#include <kernel/kernel.h>
#include <drivers/vga/vga.h>
#include <drivers/vga/fonts.h>
#include <stdlib/stdlib.h>
#include <stdlib/string.h>
#include <drivers/fs/fat/fat.h>
#include <drivers/ps2/ps2.h>
#include <drivers/rtc/rtc.h>


extern uint64_t millis_since_boot;

namespace vga {
extern PSF_header_t *font_hdr;
extern limine::limine_framebuffer *fbuf_info;
}

namespace kernel {
extern volatile limine::limine_smp_request smp_request;
extern uint64_t heapBlkcount;
extern uint64_t heapBlksize;
}

namespace wm
{
    Window *windows[16];
    int window_count = 0;
    Window *focused_window = nullptr;
    uint32_t next_window_id = 1;
    WindowInteractionState interaction_state = {false, false, nullptr, 0, 0, 0, 0};

    // Working directory for terminal windows (shared for now, could be per-window)
    char current_working_directory[256] = "/";

    void window_manager_init()
    {
        log::d("WindowManager", "Initializing window manager...");
        for (int i = 0; i < 16; i++)
        {
            windows[i] = nullptr;
        }
        window_count = 0;
        focused_window = nullptr;
        next_window_id = 1;
        log::d("WindowManager", "Window manager initialized successfully");
    }

    Window *create_window(int x, int y, uint32_t width, uint32_t height, const char *title)
    {
        log::d("WindowManager", "Creating window: %s (%dx%d at %d,%d)", title, width, height, x, y);

        if (window_count >= 16)
        {
            log::e("WindowManager", "Maximum windows reached!");
            return nullptr;
        }

        Window *window = (Window *)kernel::kmalloc(sizeof(Window));
        if (!window)
        {
            log::e("WindowManager", "Failed to allocate window structure");
            return nullptr;
        }

        // Allocate framebuffer for the window
        uint32_t buffer_size = width * height * sizeof(uint32_t);
        window->framebuffer = (uint32_t *)kernel::kmalloc(buffer_size);

        if (!window->framebuffer)
        {
            log::e("WindowManager", "Failed to allocate framebuffer (%d bytes)", buffer_size);
            kernel::kfree(window);
            return nullptr;
        }

        window->id = next_window_id++;
        window->x = x;
        window->y = y;
        window->width = width;
        window->height = height;
        window->visible = true;
        window->focused = false;
        stdlib::strcpy(window->title, title);

        // Clear the window framebuffer
        uint32_t pixel_count = width * height;
        for (uint32_t i = 0; i < pixel_count; i++)
        {
            window->framebuffer[i] = 0x00000000; // Black
        }

        // Add to windows array
        windows[window_count++] = window;
        log::d("WindowManager", "Window created successfully, ID=%d, count=%d", window->id, window_count);

        return window;
    }

    void destroy_window(Window *window)
    {
        if (!window)
            return;

        // Remove from windows array
        for (int i = 0; i < window_count; i++)
        {
            if (windows[i] == window)
            {
                for (int j = i; j < window_count - 1; j++)
                {
                    windows[j] = windows[j + 1];
                }
                windows[--window_count] = nullptr;
                break;
            }
        }

        if (focused_window == window)
        {
            focused_window = (window_count > 0) ? windows[window_count - 1] : nullptr;
            if (focused_window)
                focused_window->focused = true;
        }

        if (window->terminal_data.input_buffer)
        {
            kernel::kfree(window->terminal_data.input_buffer);
        }

        kernel::kfree(window->framebuffer);
        kernel::kfree(window);
    }

    void move_window(Window *window, int new_x, int new_y)
    {
        if (!window)
            return;
        window->x = new_x;
        window->y = new_y;
    }

    void resize_window(Window *window, uint32_t new_width, uint32_t new_height)
    {
        if (!window)
            return;

        uint32_t new_buffer_size = new_width * new_height * sizeof(uint32_t);
        uint32_t *new_framebuffer = (uint32_t *)kernel::kmalloc(new_buffer_size);
        if (!new_framebuffer)
            return;

        kernel::memset_8(new_framebuffer, new_buffer_size, 0);

        uint32_t copy_width = (new_width < window->width) ? new_width : window->width;
        uint32_t copy_height = (new_height < window->height) ? new_height : window->height;

        for (uint32_t y = 0; y < copy_height; y++)
        {
            for (uint32_t x = 0; x < copy_width; x++)
            {
                new_framebuffer[y * new_width + x] = window->framebuffer[y * window->width + x];
            }
        }

        kernel::kfree(window->framebuffer);
        window->framebuffer = new_framebuffer;
        window->width = new_width;
        window->height = new_height;

        if (window->terminal_data.input_buffer && vga::font_hdr && vga::font_hdr->width > 0 && vga::font_hdr->height > 0)
        {
            window->terminal_data.cols = new_width / vga::font_hdr->width;
            window->terminal_data.rows = new_height / vga::font_hdr->height;
        }
    }

    void set_window_focus(Window *window)
    {
        if (!window)
            return;

        if (focused_window)
        {
            focused_window->focused = false;
        }

        focused_window = window;
        window->focused = true;
    }

    void bring_window_to_top(Window *window)
    {
        if (!window)
            return;

        int window_index = -1;
        for (int i = 0; i < window_count; i++)
        {
            if (windows[i] == window)
            {
                window_index = i;
                break;
            }
        }

        if (window_index == -1)
            return;

        if (window_index == window_count - 1)
            return;

        for (int i = window_index; i < window_count - 1; i++)
        {
            windows[i] = windows[i + 1];
        }

        windows[window_count - 1] = window;
    }

    void window_putpixel(Window *window, int x, int y, vga::Color color)
    {
        if (!window || x < 0 || y < 0 || x >= (int)window->width || y >= (int)window->height)
            return;

        window->framebuffer[y * window->width + x] = color.getRGB();
    }

    void window_fillrect(Window *window, int x, int y, uint32_t w, uint32_t h, vga::Color color)
    {
        if (!window)
            return;

        for (uint32_t dy = 0; dy < h; dy++)
        {
            for (uint32_t dx = 0; dx < w; dx++)
            {
                window_putpixel(window, x + dx, y + dy, color);
            }
        }
    }

    void window_putchar(Window *window, int x, int y, char c, vga::Color fg, vga::Color bg)
    {
        if (!window || !vga::font_hdr || vga::font_hdr->width == 0 || vga::font_hdr->height == 0)
            return;

        uint8_t *bitmap = (uint8_t *)vga::font_hdr + vga::font_hdr->header_sz;
        uint8_t *glyph = (c * vga::font_hdr->glyph_size) + bitmap;

        for (int dy = 0; dy < (int)vga::font_hdr->height; dy++)
        {
            for (int dx = 0; dx < (int)vga::font_hdr->width; dx++)
            {
                if (*glyph & (1 << (vga::font_hdr->width - dx - 1)))
                {
                    window_putpixel(window, x + dx, y + dy, fg);
                }
                else
                {
                    window_putpixel(window, x + dx, y + dy, bg);
                }
            }
            glyph += vga::font_hdr->width / 8 + 1;
            if (vga::font_hdr->width % 8 == 0)
            {
                glyph--;
            }
        }
    }

    void window_clear(Window *window, vga::Color bg_color)
    {
        if (!window)
            return;

        uint32_t color_value = bg_color.getRGB();
        for (uint32_t i = 0; i < window->width * window->height; i++)
        {
            window->framebuffer[i] = color_value;
        }
    }

    void render_window(Window *window)
    {
        static int render_count = 0;
        if (!window)
        {
            if (render_count < 5)
                log::e("WindowManager", "render_window called with null window");
            return;
        }
        if (!window->visible)
        {
            if (render_count < 5)
                log::d("WindowManager", "Window %d is not visible", window->id);
            return;
        }

        if (render_count < 5)
        {
            // log::d("WindowManager", "Rendering window %d: %s (%dx%d at %d,%d)",
            //        window->id, window->title, window->width, window->height, window->x, window->y);
            render_count++;
        }

        // Draw window border (simple 2px border)
        vga::Color border_color = window->focused ? vga::Color(0, 255, 255) : vga::Color(128, 128, 128);

        // Top border
        vga::fillRect(window->x - 2, window->y - 22, border_color, window->width + 4, 2);
        // Bottom border
        vga::fillRect(window->x - 2, window->y + window->height, border_color, window->width + 4, 2);
        // Left border
        vga::fillRect(window->x - 2, window->y - 20, border_color, 2, window->height + 20);
        // Right border
        vga::fillRect(window->x + window->width, window->y - 20, border_color, 2, window->height + 20);

        // Title bar
        vga::fillRect(window->x - 2, window->y - 20, vga::Color(64, 64, 64), window->width + 4, 18);

        // Window title with function key label
        if (vga::font_hdr && vga::font_hdr->width > 0)
        {
            int title_x = window->x;
            int title_y = window->y - 20;
            
            // Find window index for function key label
            int window_index = -1;
            for (int i = 0; i < window_count; i++)
            {
                if (windows[i] == window)
                {
                    window_index = i;
                    break;
                }
            }
            
            // Draw function key label if window index is valid (F1-F12)
            if (window_index >= 0 && window_index < 12)
            {
                char f_key_label[8];
                if (window_index < 9)
                {
                    f_key_label[0] = 'F';
                    f_key_label[1] = '1' + window_index;
                    f_key_label[2] = ':';
                    f_key_label[3] = ' ';
                    f_key_label[4] = '\0';
                }
                else
                {
                    f_key_label[0] = 'F';
                    f_key_label[1] = '1';
                    f_key_label[2] = '0' + (window_index - 9);
                    f_key_label[3] = ':';
                    f_key_label[4] = ' ';
                    f_key_label[5] = '\0';
                }
                
                // Draw function key label
                int label_len = stdlib::strlen(f_key_label);
                for (int i = 0; i < label_len; i++)
                {
                    vga::putchar(title_x + i * vga::font_hdr->width, title_y + 2, f_key_label[i]);
                }
                title_x += label_len * vga::font_hdr->width;
            }
            
            // Draw window title
            for (int i = 0; window->title[i] != '\0' && i < 15; i++)
            {
                vga::putchar(title_x + i * vga::font_hdr->width, title_y + 2, window->title[i]);
            }
        }

        // Copy window framebuffer to screen
        for (uint32_t y = 0; y < window->height; y++)
        {
            for (uint32_t x = 0; x < window->width; x++)
            {
                int screen_x = window->x + x;
                int screen_y = window->y + y;

                if (screen_x >= 0 && screen_y >= 0 &&
                    screen_x < (int)vga::fbuf_info->width &&
                    screen_y < (int)vga::fbuf_info->height)
                {
                    vga::putpixel(screen_x, screen_y, vga::Color(window->framebuffer[y * window->width + x]));
                }
            }
        }
    }

    void render_all_windows()
    {
        static int render_count = 0;
        if (render_count < 3) // Only log first few renders
        {
            log::d("WindowManager", "Rendering all windows (count=%d)", window_count);
            render_count++;
        }

        vga::clearScreen();
        vga::fillRect(0, 0, vga::Color(0, 150, 200), vga::fbuf_info->width, vga::fbuf_info->height);

        // Render all windows in order
        for (int i = 0; i < window_count; i++)
        {
            if (render_count <= 5)
            {
                // log::d("WindowManager", "Rendering window %d", i);
            }
            render_window(windows[i]);
        }

        // Draw mouse cursor (NOT IMPLEMENTED YET)
        // draw_mouse_cursor();

        vga::g_framebuffer_dirty = true;
    }

    Window *create_terminal_window(int x, int y, uint32_t width, uint32_t height)
    {
        log::d("WindowManager", "Creating terminal window...");
        Window *window = create_window(x, y, width, height, "Terminal");
        if (!window)
        {
            log::e("WindowManager", "Failed to create base window for terminal");
            return nullptr;
        }

        // Safety check for font initialization
        if (!vga::font_hdr || vga::font_hdr->width == 0 || vga::font_hdr->height == 0)
        {
            log::e("WindowManager", "Font not initialized! vga::font_hdr=%x, width=%d, height=%d",
                   (uint64_t)vga::font_hdr, vga::font_hdr ? vga::font_hdr->width : 0, vga::font_hdr ? vga::font_hdr->height : 0);
            destroy_window(window);
            return nullptr;
        }

        log::d("WindowManager", "Font OK: %dx%d", vga::font_hdr->width, vga::font_hdr->height);

        // Initialize terminal-specific data
        window->terminal_data.cursor_x = 0;
        window->terminal_data.cursor_y = 0;
        window->terminal_data.cols = width / vga::font_hdr->width;
        window->terminal_data.rows = height / vga::font_hdr->height;
        window->terminal_data.fg_color = vga::Color(255, 255, 255);
        window->terminal_data.bg_color = vga::Color(0, 0, 0);

        // Allocate input buffer
        int buffer_size = window->terminal_data.cols * window->terminal_data.rows + 69;
        log::d("WindowManager", "Allocating terminal input buffer: %d bytes for %dx%d terminal",
               buffer_size, window->terminal_data.cols, window->terminal_data.rows);
        window->terminal_data.input_buffer = (char *)kernel::kmalloc(buffer_size);
        window->terminal_data.input_pos = 0;

        if (!window->terminal_data.input_buffer)
        {
            log::e("WindowManager", "Failed to allocate terminal input buffer");
            destroy_window(window);
            return nullptr;
        }

        // Clear window with black background
        log::d("WindowManager", "Clearing terminal window background");
        window_clear(window, window->terminal_data.bg_color);

        // Add welcome message and prompt
        terminal_window_puts(window, "Welcome to NEO-OS Terminal!\n");
        terminal_window_puts(window, "Type 'help' for available commands\n");
        terminal_window_print_prompt(window);

        window->type = WINDOW_TYPE_TERMINAL;
        log::d("WindowManager", "Terminal window created successfully");
        return window;
    }

    Window *create_login_window(int x, int y, uint32_t width, uint32_t height)
    {
        log::d("WindowManager", "Creating login window...");
        Window *window = create_window(x, y, width, height, "Login");
        if (!window)
        {
            log::e("WindowManager", "Failed to create base window for login");
            return nullptr;
        }

        window->type = WINDOW_TYPE_LOGIN;

        // Initialize login-specific data (reuse terminal data structure)
        window->terminal_data.cursor_x = 0;
        window->terminal_data.cursor_y = 0;
        window->terminal_data.cols = width / vga::font_hdr->width;
        window->terminal_data.rows = height / vga::font_hdr->height;
        window->terminal_data.fg_color = vga::Color(255, 255, 255);
        window->terminal_data.bg_color = vga::Color(0, 0, 0);

        // Allocate input buffer for password
        int buffer_size = 256; // Reasonable size for password
        window->terminal_data.input_buffer = (char *)kernel::kmalloc(buffer_size);
        window->terminal_data.input_pos = 0;

        if (!window->terminal_data.input_buffer)
        {
            log::e("WindowManager", "Failed to allocate login input buffer");
            destroy_window(window);
            return nullptr;
        }

        // Clear window with black background
        window_clear(window, window->terminal_data.bg_color);

        // Display login prompt
        terminal_window_puts(window, "NEO-OS Login\n");
        terminal_window_puts(window, "============\n\n");
        terminal_window_puts(window, "Password: ");

        log::d("WindowManager", "Login window created successfully");
        return window;
    }

    void login_window_handle_input(Window *window, char key)
    {
        if (!window || window->type != WINDOW_TYPE_LOGIN)
            return;

        switch (key)
        {
        case '\n':
        case '\r':
        {
            // Process login attempt
            window->terminal_data.input_buffer[window->terminal_data.input_pos] = '\0';

            // Read password from /etc/login file
            kernel::File password_file;
            stdlib::string password_path = "/etc/login";
            int ret = kernel::open(&password_file, &password_path);

            if (ret == -1)
            {
                terminal_window_puts(window, "\nError: Cannot read password file\n");
                terminal_window_puts(window, "Password: ");
                // Clear input
                for (int i = 0; i < window->terminal_data.input_pos; i++)
                    window->terminal_data.input_buffer[i] = '\0';
                window->terminal_data.input_pos = 0;
                return;
            }

            char *stored_password = (char *)kernel::read(&password_file);
            kernel::close(&password_file);

            if (stored_password && stdlib::strcmp(stored_password, window->terminal_data.input_buffer))
            {
                // Login successful
                terminal_window_printf(window, "\n%pLogin successful!%p\n",
                                       vga::Color(0, 255, 0).getRGB(),
                                       vga::Color(255, 255, 255).getRGB());

                // Clean up password from memory
                if (stored_password)
                    kernel::free_pages(stored_password);

                // Hide login window and create terminal
                window->visible = false;

                // Create main terminal window
                Window *main_terminal = create_terminal_window(100, 100, 600, 400);
                if (main_terminal)
                {
                    set_window_focus(main_terminal);
                }
            }
            else
            {
                // Login failed
                terminal_window_printf(window, "\n%pPassword incorrect :(%p\n",
                                       vga::Color(255, 0, 0).getRGB(),
                                       vga::Color(255, 255, 255).getRGB());
                terminal_window_puts(window, "Password: ");

                // Clean up password from memory
                if (stored_password)
                    kernel::free_pages(stored_password);
            }

            // Clear input buffer
            for (int i = 0; i < window->terminal_data.input_pos; i++)
                window->terminal_data.input_buffer[i] = '\0';
            window->terminal_data.input_pos = 0;
            break;
        }

        case '\b':
            // Backspace
            if (window->terminal_data.input_pos > 0)
            {
                window->terminal_data.input_pos--;
                window->terminal_data.input_buffer[window->terminal_data.input_pos] = '\0';

                // Move cursor back and clear character (show asterisk removal)
                if (window->terminal_data.cursor_x > 0)
                {
                    window->terminal_data.cursor_x--;
                    window_putchar(window, window->terminal_data.cursor_x * vga::font_hdr->width,
                                   window->terminal_data.cursor_y * vga::font_hdr->height, ' ',
                                   window->terminal_data.fg_color, window->terminal_data.bg_color);
                }
            }
            break;

        default:
            // Add character to password (but display asterisk)
            if (key >= 32 && key <= 126) // Printable characters
            {
                int buffer_size = 256;
                if (window->terminal_data.input_pos < buffer_size - 1)
                {
                    window->terminal_data.input_buffer[window->terminal_data.input_pos++] = key;
                    window->terminal_data.input_buffer[window->terminal_data.input_pos] = '\0';

                    // Display asterisk instead of actual character
                    terminal_window_putc(window, '*', false);
                }
            }
            break;
        }
    }

    void terminal_window_scroll_up(Window *window)
    {
        if (!window || !window->terminal_data.input_buffer || !vga::font_hdr || vga::font_hdr->height == 0)
            return;

        int font_height = vga::font_hdr->height;

        // Copy each line up by font_height pixels
        for (int y = font_height; y < (int)window->height; y++)
        {
            for (int x = 0; x < (int)window->width; x++)
            {
                window->framebuffer[(y - font_height) * window->width + x] =
                    window->framebuffer[y * window->width + x];
            }
        }

        // Clear the bottom line
        for (int y = window->height - font_height; y < (int)window->height; y++)
        {
            for (int x = 0; x < (int)window->width; x++)
            {
                window->framebuffer[y * window->width + x] = window->terminal_data.bg_color.getRGB();
            }
        }
    }

    void terminal_window_draw_cursor(Window *window, int x, int y)
    {
        if (!window || !vga::font_hdr || vga::font_hdr->width == 0 || vga::font_hdr->height == 0)
            return;

        int pixel_x = x * vga::font_hdr->width;
        int pixel_y = y * vga::font_hdr->height;

        window_fillrect(window, pixel_x, pixel_y, vga::font_hdr->width, vga::font_hdr->height,
                        window->terminal_data.fg_color);
    }

    void terminal_window_clear_cursor(Window *window, int x, int y)
    {
        if (!window || !vga::font_hdr || vga::font_hdr->width == 0 || vga::font_hdr->height == 0)
            return;

        int pixel_x = x * vga::font_hdr->width;
        int pixel_y = y * vga::font_hdr->height;

        window_fillrect(window, pixel_x, pixel_y, vga::font_hdr->width, vga::font_hdr->height,
                        window->terminal_data.bg_color);
    }

    void terminal_window_putc(Window *window, char src, bool is_input)
    {
        if (!window || !window->terminal_data.input_buffer)
            return;

        switch (src)
        {
        case '\r':
        case '\0':WINDOW_TYPE_SYSINFO
            break;
        case '\n':
            // Clear cursor
            terminal_window_clear_cursor(window, window->terminal_data.cursor_x, window->terminal_data.cursor_y);

            // Check if we need to scroll
            if (window->terminal_data.cursor_y >= window->terminal_data.rows - 1)
            {
                terminal_window_scroll_up(window);
                window->terminal_data.cursor_x = 0;
            }
            else
            {
                window->terminal_data.cursor_y++;
                window->terminal_data.cursor_x = 0;
            }

            // Draw cursor
            terminal_window_draw_cursor(window, window->terminal_data.cursor_x, window->terminal_data.cursor_y);

            if (is_input)
            {
                // Process command and then show new prompt
                if (window->terminal_data.input_pos > 0)
                {
                    window->terminal_data.input_buffer[window->terminal_data.input_pos] = '\0';
                    terminal_window_run_command(window, window->terminal_data.input_buffer);
                }
                else
                {
                    // Just show prompt if no command entered
                    terminal_window_print_prompt(window);
                }
            }
            break;

        case '\t':
            // Clear cursor
            terminal_window_clear_cursor(window, window->terminal_data.cursor_x, window->terminal_data.cursor_y);

            window->terminal_data.cursor_x += 4 - (window->terminal_data.cursor_x % 4);

            // Draw cursor
            terminal_window_draw_cursor(window, window->terminal_data.cursor_x, window->terminal_data.cursor_y);

            if (is_input)
            {
                int buffer_size = window->terminal_data.cols * window->terminal_data.rows + 69;
                int spaces_to_add = 4 - (window->terminal_data.input_pos % 4);

                if (window->terminal_data.input_pos + spaces_to_add < buffer_size)
                {
                    for (int i = 0; i < spaces_to_add; i++)
                    {
                        window->terminal_data.input_buffer[window->terminal_data.input_pos + i] = ' ';
                    }
                    window->terminal_data.input_pos += spaces_to_add;
                    window->terminal_data.input_buffer[window->terminal_data.input_pos] = '\0';
                }
            }
            break;

        case '\b':
            if (is_input)
            {
                if (window->terminal_data.input_pos == 0)
                    break;

                window->terminal_data.input_pos--;
                window->terminal_data.input_buffer[window->terminal_data.input_pos] = '\0';
            }

            if (window->terminal_data.cursor_x > 0)
            {
                // Clear cursor
                terminal_window_clear_cursor(window, window->terminal_data.cursor_x, window->terminal_data.cursor_y);

                window->terminal_data.cursor_x--;

                // Draw cursor
                terminal_window_draw_cursor(window, window->terminal_data.cursor_x, window->terminal_data.cursor_y);
            }
            break;

        default:
            // Draw character (with safety check)
            if (!vga::font_hdr || vga::font_hdr->width == 0 || vga::font_hdr->height == 0)
                break;

            int pixel_x = window->terminal_data.cursor_x * vga::font_hdr->width;
            int pixel_y = window->terminal_data.cursor_y * vga::font_hdr->height;

            window_putchar(window, pixel_x, pixel_y, src,
                           window->terminal_data.fg_color, window->terminal_data.bg_color);

            if (is_input)
            {
                // Check bounds before writing to input buffer
                int buffer_size = window->terminal_data.cols * window->terminal_data.rows + 69;
                if (window->terminal_data.input_pos < buffer_size - 1)
                {
                    window->terminal_data.input_buffer[window->terminal_data.input_pos++] = src;
                    window->terminal_data.input_buffer[window->terminal_data.input_pos] = '\0';
                }
            }

            // Move cursor
            window->terminal_data.cursor_x++;
            terminal_window_draw_cursor(window, window->terminal_data.cursor_x, window->terminal_data.cursor_y);
            break;
        }
    }

    void terminal_window_puts(Window *window, const char *str)
    {
        if (!window || !str)
            return;

        while (*str != '\0')
        {
            terminal_window_putc(window, *str++);
        }
    }

    void terminal_window_printf(Window *window, const char *fmt, ...)
    {
        if (!window || !fmt)
            return;

        va_list args;
        va_start(args, fmt);

        int current = 0;
        void *token;
        uint64_t num;
        bool argFound;

        while (fmt[current] != '\0')
        {
            if (fmt[current] != '%')
            {
                terminal_window_putc(window, fmt[current++]);
                continue;
            }

            switch (fmt[current + 1])
            {
            case 'c': // Char
            case 'C':
                argFound = true;
                num = va_arg(args, int);
                terminal_window_putc(window, num);
                break;
            case 's': // String
            case 'S':
                argFound = true;
                token = va_arg(args, void *);
                terminal_window_puts(window, (char *)token);
                break;
            case 'u': // Unsigned Integer
            case 'U':
                argFound = true;
                num = va_arg(args, uint64_t);
                token = stdlib::utoa(num, 10);
                terminal_window_puts(window, (char *)token);
                break;
            case 'd':
            case 'D': // Signed integer
                argFound = true;
                num = va_arg(args, int64_t);
                token = stdlib::itoa(num, 10);
                terminal_window_puts(window, (char *)token);
                break;
            case 'x': // Hexadecimal unsigned int
            case 'X':
                argFound = true;
                num = va_arg(args, uint64_t);
                token = stdlib::utoa(num, 16);
                terminal_window_puts(window, (char *)token);
                break;
            case 'p': // color
                num = va_arg(args, uint32_t);
                window->terminal_data.fg_color = vga::Color(num);
                argFound = true;
                break;
            case 'a':
            case 'A': // color background
                terminal_window_draw_cursor(window, window->terminal_data.cursor_x, window->terminal_data.cursor_y);
                window->terminal_data.cursor_x++;
                argFound = true;
                break;
            default:
                argFound = false;
            }

            if (argFound)
            {
                current += 2;
                continue;
            }

            current++;
        }

        va_end(args);
    }

    // Helper function to resolve relative paths to absolute paths
    stdlib::string resolve_path(const char *path_input)
    {
        if (path_input == nullptr || stdlib::strlen(path_input) == 0)
        {
            return stdlib::string(current_working_directory);
        }

        // If path starts with '/', it's already absolute
        if (path_input[0] == '/')
        {
            return stdlib::string(path_input);
        }

        // Build absolute path from current directory and relative path
        stdlib::string absolute_path(current_working_directory);

        // Add trailing slash if needed
        if (absolute_path.c_str()[absolute_path.length() - 1] != '/')
        {
            absolute_path.push_back('/');
        }

        // Append the relative path
        stdlib::string relative(path_input);
        absolute_path.append(relative);

        return absolute_path;
    }

    // Helper function to change directory
    bool change_directory(const char *path)
    {
        // Handle special cases
        if (path != nullptr && stdlib::strcmp(path, "."))
        {
            return true; // Current directory - do nothing
        }

        if (path != nullptr && stdlib::strcmp(path, ".."))
        {
            // Parent directory - go up one level
            size_t len = stdlib::strlen(current_working_directory);

            // If we're at root, stay at root
            if (len <= 1 || (len == 1 && current_working_directory[0] == '/'))
            {
                return false;
            }

            // Find the last '/' and truncate there
            for (int i = len - 1; i >= 0; i--)
            {
                if (current_working_directory[i] == '/')
                {
                    // If this is the root slash, keep it
                    if (i == 0)
                    {
                        current_working_directory[1] = '\0';
                    }
                    else
                    {
                        current_working_directory[i] = '\0';
                    }
                    break;
                }
            }
            return true;
        }

        if (path[0] == '/' && path[1] == '\0')
        {
            current_working_directory[0] = '/';
            current_working_directory[1] = '\0';
            return true;
        }

        // Regular path - resolve and validate
        stdlib::string abs_path = resolve_path(path);

        // Try to open the path to verify it exists and is a directory
        kernel::File test_file;
        int ret = kernel::open(&test_file, &abs_path);

        if (ret == -1)
        {
            return false; // Path doesn't exist
        }

        if (!test_file.is_dir)
        {
            kernel::close(&test_file);
            return false; // Path is not a directory
        }

        kernel::close(&test_file);

        // Update current working directory
        stdlib::strcpy(current_working_directory, abs_path.c_str());
        return true;
    }

    void terminal_window_print_prompt(Window *window)
    {
        if (!window)
            return;

        // Print colorized prompt exactly like original terminal: ROOT@NEO-OS:/path$
        terminal_window_printf(window, "%pROOT@NEO-OS%p:%p%s%p$%p ",
                               vga::Color(0, 255, 0).getRGB(), vga::Color(0, 0, 255).getRGB(),
                               vga::Color(150, 150, 150).getRGB(), current_working_directory,
                               vga::Color(0, 0, 255).getRGB(), vga::Color(255, 255, 255).getRGB());
    }

    void terminal_window_clear_input_buffer(Window *window)
    {
        if (!window || !window->terminal_data.input_buffer)
            return;

        for (int i = 0; i < window->terminal_data.input_pos; i++)
        {
            window->terminal_data.input_buffer[i] = '\0';
        }
        window->terminal_data.input_pos = 0;
    }

    void terminal_window_clear(Window *window)
    {
        if (!window)
            return;

        window->terminal_data.cursor_x = 0;
        window->terminal_data.cursor_y = 0;
        window_clear(window, window->terminal_data.bg_color);
    }

    void terminal_window_run_command(Window *window, const char *command)
    {
        if (!window || !command)
            return;

        // Skip login check for now - assume we're logged in

        stdlib::string command_str(command);
        int count;
        stdlib::string **sp = command_str.split(' ', &count);

        if (stdlib::strcmp(sp[0]->c_str(), "help"))
        {
            terminal_window_puts(window, "Available commands:\n");
            terminal_window_puts(window, "help - Show this help message\n");
            terminal_window_puts(window, "clear - Clear the screen\n");
            terminal_window_puts(window, "ls [path] - List files in directory\n");
            terminal_window_puts(window, "cat [file] - Display file contents\n");
            terminal_window_puts(window, "pwd - Print current working directory\n");
            terminal_window_puts(window, "cd [path] - Change working directory\n");
            terminal_window_puts(window, "fetch - Display system information\n");
            terminal_window_puts(window, "open - Open a new terminal window\n");
            terminal_window_puts(window, "exit - Close this terminal window\n");
            terminal_window_puts(window, "clock - Open a digital clock window\n");
            terminal_window_puts(window, "sysinfo - Open system information window\n");
            terminal_window_puts(window, "snake - Play the Snake game\n");
            terminal_window_puts(window, "\nWindow controls:\n");
            terminal_window_puts(window, "Arrow keys - Move active window\n");
            terminal_window_puts(window, "F1-F11 - Switch to window by function key\n");
            terminal_window_puts(window, "F12 - Close active window\n");
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "clear"))
        {
            terminal_window_clear(window);
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "ls"))
        {
            // Use current directory if no path specified, otherwise resolve the path
            const char *path = (count > 1) ? sp[1]->c_str() : nullptr;
            stdlib::string resolved_path = resolve_path(path);
            kernel::list_files(resolved_path.c_str(), window);
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "cat"))
        {
            if (count < 2)
            {
                terminal_window_puts(window, "cat: missing file argument\n");
            }
            else
            {
                stdlib::string resolved_path = resolve_path(sp[1]->c_str());
                terminal_window_print_file_contents(window, resolved_path.c_str());
            }
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "pwd"))
        {
            terminal_window_puts(window, current_working_directory);
            terminal_window_puts(window, "\n");
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "cd"))
        {
            const char *path = (count > 1) ? sp[1]->c_str() : "/";
            if (!change_directory(path))
            {
                terminal_window_puts(window, "cd: directory not found or not a directory\n");
            }
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "fetch"))
        {
            terminal_window_display_fetch(window);
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "open"))
        {
            // Create a new terminal window
            int new_x = (window_count * 30) % 400 + 50;
            int new_y = (window_count * 30) % 300 + 50;
            Window *new_terminal = create_terminal_window(new_x, new_y, 600, 400);
            if (new_terminal)
            {
                set_window_focus(new_terminal);
                terminal_window_puts(window, "New terminal window opened\n");
            }
            else
            {
                terminal_window_puts(window, "Failed to create new terminal window\n");
            }
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "exit"))
        {
            // Close current terminal window
            terminal_window_puts(window, "Closing terminal window...\n");
            // Add a small delay to show the message
            for (volatile int i = 0; i < 10000000; i++);
            destroy_window(window);
            return; // Important: return immediately since window is destroyed
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "clock"))
        {
            // Create a new clock window
            int new_x = (window_count * 20) % 300 + 100;
            int new_y = (window_count * 20) % 200 + 80;
            Window *clock_window = create_clock_window(new_x, new_y, 220, 120);
            if (clock_window)
            {
                set_window_focus(clock_window);
                terminal_window_puts(window, "Digital clock window opened\n");
            }
            else
            {
                terminal_window_puts(window, "Failed to create clock window\n");
            }
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "sysinfo"))
        {
            // Create a new system info window
            int new_x = (window_count * 25) % 250 + 50;
            int new_y = (window_count * 25) % 150 + 50;
            Window *sysinfo_window = create_sysinfo_window(new_x, new_y, 400, 320);
            if (sysinfo_window)
            {
                set_window_focus(sysinfo_window);
                terminal_window_puts(window, "System information window opened\n");
            }
            else
            {
                terminal_window_puts(window, "Failed to create system info window\n");
            }
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "snake"))
        {
            // Create a new snake game window
            int new_x = (window_count * 30) % 200 + 50;
            int new_y = (window_count * 30) % 100 + 50;
            Window *snake_window = create_snake_window(new_x, new_y, 300, 240);
            if (snake_window)
            {
                set_window_focus(snake_window);
                terminal_window_puts(window, "Snake game started! Use WASD keys.\n");
            }
            else
            {
                terminal_window_puts(window, "Failed to create snake game window\n");
            }
        }
        else if (stdlib::strlen(sp[0]->c_str()) > 0)
        {
            terminal_window_puts(window, "Command not found: ");
            terminal_window_puts(window, sp[0]->c_str());
            terminal_window_puts(window, "\n");
        }

        terminal_window_clear_input_buffer(window);
        terminal_window_print_prompt(window);
    }

    void terminal_window_print_file_contents(Window *window, const char *path)
    {
        kernel::File file;
        stdlib::string file_path = path;
        int ret = kernel::open(&file, &file_path);

        if (ret == -1)
        {
            terminal_window_puts(window, "cat: cannot access '");
            terminal_window_puts(window, path);
            terminal_window_puts(window, "': No such file or directory\n");
            return;
        }

        if (file.is_dir)
        {
            terminal_window_puts(window, "cat: '");
            terminal_window_puts(window, path);
            terminal_window_puts(window, "': Is a directory\n");
            kernel::close(&file);
            return;
        }

        char *text = (char *)kernel::read(&file);
        if (text == nullptr)
        {
            terminal_window_puts(window, "cat: error reading file\n");
        }
        else
        {
            terminal_window_puts(window, text);
            terminal_window_puts(window, "\n");
            kernel::free_pages(text);
        }
        kernel::close(&file);
    }

    void terminal_window_display_fetch(Window *window)
    {
        // Display neo-OS ASCII art and system info (windowed version)
        terminal_window_puts(window, "\n");

        // Enhanced ASCII art logo for neo-OS
        terminal_window_printf(window, "%p    ####   ##  ######   #######           #######   ######    %p\n",
                               vga::Color(0, 255, 255).getRGB(),
                               vga::Color(255, 255, 255).getRGB());
        terminal_window_printf(window, "%p    ## ##  ##  ##       ##   ##  ######   ##   ##  ##         %p\n",
                               vga::Color(0, 220, 255).getRGB(),
                               vga::Color(255, 255, 255).getRGB());
        terminal_window_printf(window, "%p    ##  ## ##  ####     ##   ##           ##   ##   ####      %p\n",
                               vga::Color(0, 190, 255).getRGB(),
                               vga::Color(255, 255, 255).getRGB());
        terminal_window_printf(window, "%p    ##   ####  ##       ##   ##           ##   ##       ##    %p\n",
                               vga::Color(0, 160, 255).getRGB(),
                               vga::Color(255, 255, 255).getRGB());
        terminal_window_printf(window, "%p    ##    ###  ######   #######           #######   ######    %p\n\n",
                               vga::Color(0, 130, 255).getRGB(),
                               vga::Color(255, 255, 255).getRGB());

        // System information
        terminal_window_printf(window, "%pOS:%p NEO-OS v0.001A\n",
                               vga::Color(255, 100, 100).getRGB(),
                               vga::Color(255, 255, 255).getRGB());

        terminal_window_printf(window, "%pArchitecture:%p x86_64\n",
                               vga::Color(255, 100, 100).getRGB(),
                               vga::Color(255, 255, 255).getRGB());

        terminal_window_printf(window, "%pTerminal:%p Windowed Terminal\n",
                               vga::Color(255, 100, 100).getRGB(),
                               vga::Color(255, 255, 255).getRGB());

        terminal_window_printf(window, "%pShell:%p NEO Shell\n",
                               vga::Color(255, 100, 100).getRGB(),
                               vga::Color(255, 255, 255).getRGB());

        // CPU info
        if (kernel::smp_request.response != nullptr)
        {
            terminal_window_printf(window,"%pCPU:%p %u cores\n", vga::Color(255, 100, 100).getRGB(),
                           vga::Color(255, 255, 255).getRGB(),
                           kernel::smp_request.response->cpu_count);
        }
        else
        {
            terminal_window_printf(window,"%pCPU:%p Unknown\n", vga::Color(255, 100, 100).getRGB(),
                           vga::Color(255, 255, 255).getRGB());
        }

        // Memory info
        uint64_t total_heap_mb = (kernel::heapBlkcount * kernel::heapBlksize) / (1024 * 1024);
        terminal_window_printf(window,"%pMemory:%p %u MB heap allocated\n",
                       vga::Color(255, 100, 100).getRGB(),
                       vga::Color(255, 255, 255).getRGB(), total_heap_mb);
        // Display resolution
        if (vga::fbuf_info != nullptr)
        {
            terminal_window_printf(window,"%pResolution:%p %ux%u\n",
                           vga::Color(255, 100, 100).getRGB(),
                           vga::Color(255, 255, 255).getRGB(), vga::fbuf_info->width,
                           vga::fbuf_info->height);
        }

        // Font information
        if (vga::font_hdr != nullptr)
        {
            terminal_window_printf(window,"%pFont:%p PSF %ux%u (%u glyphs, %u bytes/glyph)\n",
                           vga::Color(255, 100, 100).getRGB(),
                           vga::Color(255, 255, 255).getRGB(),
                           vga::font_hdr->width,
                           vga::font_hdr->height,
                           vga::font_hdr->glyph_count,
                           vga::font_hdr->glyph_size);
        }
        else
        {
            terminal_window_printf(window,"%pFont:%p Unknown\n",
                           vga::Color(255, 100, 100).getRGB(),
                           vga::Color(255, 255, 255).getRGB());
        }

        // Uptime
        uint64_t uptime_seconds = millis_since_boot / 1000;
        uint64_t hours = uptime_seconds / 3600;
        uint64_t minutes = (uptime_seconds % 3600) / 60;
        uint64_t seconds = uptime_seconds % 60;

        terminal_window_printf(window,"%pUptime:%p %uh %um %us\n",
                       vga::Color(255, 100, 100).getRGB(),
                       vga::Color(255, 255, 255).getRGB(), hours, minutes, seconds);

        // Color blocks for terminal colors demonstration
        terminal_window_printf(window,"%pColors:%p ", vga::Color(255, 100, 100).getRGB(),
                       vga::Color(255, 255, 255).getRGB());
        terminal_window_printf(window,"%p%a%p", vga::Color(255, 0, 0).getRGB(),
                       vga::Color(255, 255, 255).getRGB());
        terminal_window_printf(window,"%p%a%p", vga::Color(0, 255, 0).getRGB(),
                       vga::Color(255, 255, 255).getRGB());
        terminal_window_printf(window,"%p%a%p", vga::Color(0, 0, 255).getRGB(),
                       vga::Color(255, 255, 255).getRGB());
        terminal_window_printf(window,"%p%a%p", vga::Color(255, 255, 0).getRGB(),
                       vga::Color(255, 255, 255).getRGB());
        terminal_window_printf(window,"%p%a%p", vga::Color(255, 0, 255).getRGB(),
                       vga::Color(255, 255, 255).getRGB());
        terminal_window_printf(window,"%p%a%p", vga::Color(0, 255, 255).getRGB(),
                       vga::Color(255, 255, 255).getRGB());
        terminal_window_printf(window,"%p\n\n", vga::Color(255, 255, 255).getRGB());

        terminal_window_puts(window, "\n");
    }

    Window *create_clock_window(int x, int y, uint32_t width, uint32_t height)
    {
        log::d("WindowManager", "Creating clock window...");
        Window *window = create_window(x, y, width, height, "Digital Clock");
        if (!window)
        {
            log::e("WindowManager", "Failed to create base window for clock");
            return nullptr;
        }

        window->type = WINDOW_TYPE_CLOCK;

        // Clear window with black background
        window_clear(window, vga::Color(0, 0, 0));

        // Initial clock display update
        update_clock_display(window);

        log::d("WindowManager", "Clock window created successfully");
        return window;
    }

    void update_clock_display(Window *window)
    {
        if (!window || window->type != WINDOW_TYPE_CLOCK)
            return;

        // Clear the window
        window_clear(window, vga::Color(0, 0, 0));

        // Get current time from RTC
        rtc::DateTime dt = rtc::get_datetime();

        // Format the time string manually
        char time_str[16];
        char date_str[16];
        
        // Format time as HH:MM:SS
        time_str[0] = '0' + (dt.hour / 10);
        time_str[1] = '0' + (dt.hour % 10);
        time_str[2] = ':';
        time_str[3] = '0' + (dt.minute / 10);
        time_str[4] = '0' + (dt.minute % 10);
        time_str[5] = ':';
        time_str[6] = '0' + (dt.second / 10);
        time_str[7] = '0' + (dt.second % 10);
        time_str[8] = '\0';
        
        // Format date as MM/DD/YYYY
        date_str[0] = '0' + (dt.month / 10);
        date_str[1] = '0' + (dt.month % 10);
        date_str[2] = '/';
        date_str[3] = '0' + (dt.day / 10);
        date_str[4] = '0' + (dt.day % 10);
        date_str[5] = '/';
        date_str[6] = '0' + ((dt.year / 1000) % 10);
        date_str[7] = '0' + ((dt.year / 100) % 10);
        date_str[8] = '0' + ((dt.year / 10) % 10);
        date_str[9] = '0' + (dt.year % 10);
        date_str[10] = '\0';

        if (!vga::font_hdr || vga::font_hdr->width == 0 || vga::font_hdr->height == 0)
            return;

        // Calculate center position for time
        int time_len = stdlib::strlen(time_str);
        int date_len = stdlib::strlen(date_str);
        int time_x = (window->width - (time_len * vga::font_hdr->width * 2)) / 2; // Double size
        int date_x = (window->width - (date_len * vga::font_hdr->width)) / 2;
        int time_y = (window->height - (vga::font_hdr->height * 3)) / 2; // Space for time and date
        int date_y = time_y + vga::font_hdr->height * 2 + 10;

        // Draw time in large font (2x scale)
        for (int i = 0; i < time_len; i++)
        {
            for (int py = 0; py < vga::font_hdr->height * 2; py++)
            {
                for (int px = 0; px < vga::font_hdr->width * 2; px++)
                {
                    // Simple 2x scaling by reading original pixel
                    int orig_x = px / 2;
                    int orig_y = py / 2;
                    
                    // Get bitmap data
                    uint8_t *bitmap = (uint8_t *)vga::font_hdr + vga::font_hdr->header_sz;
                    uint8_t *glyph = (time_str[i] * vga::font_hdr->glyph_size) + bitmap;
                    uint8_t *glyph_line = glyph + (orig_y * (vga::font_hdr->width / 8 + 1));
                    if (vga::font_hdr->width % 8 == 0)
                        glyph_line = glyph + (orig_y * (vga::font_hdr->width / 8));
                    
                    if (*glyph_line & (1 << (vga::font_hdr->width - orig_x - 1)))
                    {
                        window_putpixel(window, time_x + i * vga::font_hdr->width * 2 + px, 
                                      time_y + py, vga::Color(0, 255, 255)); // Cyan
                    }
                }
            }
        }

        // Draw date in normal size
        for (int i = 0; i < date_len; i++)
        {
            window_putchar(window, date_x + i * vga::font_hdr->width, date_y, 
                         date_str[i], vga::Color(255, 255, 255), vga::Color(0, 0, 0));
        }

        // Draw a border around the time display
        vga::Color border_color = vga::Color(100, 100, 100);
        window_fillrect(window, 10, 10, window->width - 20, 2, border_color); // Top
        window_fillrect(window, 10, window->height - 12, window->width - 20, 2, border_color); // Bottom
        window_fillrect(window, 10, 10, 2, window->height - 20, border_color); // Left
        window_fillrect(window, window->width - 12, 10, 2, window->height - 20, border_color); // Right
    }

    Window *create_sysinfo_window(int x, int y, uint32_t width, uint32_t height)
    {
        log::d("WindowManager", "Creating system info window...");
        Window *window = create_window(x, y, width, height, "neo-OS System Information");
        if (!window)
        {
            log::e("WindowManager", "Failed to create base window for system info");
            return nullptr;
        }

        window->type = WINDOW_TYPE_SYSINFO;

        // Clear window with dark blue background
        window_clear(window, vga::Color(20, 40, 80));

        // Initial display render
        render_sysinfo_display(window);

        log::d("WindowManager", "System info window created successfully");
        return window;
    }

    void draw_neo_logo(Window* window, int start_x, int start_y)
    {
        if (!window || !vga::font_hdr || vga::font_hdr->width == 0 || vga::font_hdr->height == 0)
            return;

        // neo-OS ASCII logo - smaller version for the window
        const char* logo_lines[] = {
            "   ####   ##  ######   #######",
            "   ## ##  ##  ##       ##   ##",
            "   ##  ## ##  ####     ##   ##", 
            "   ##   ####  ##       ##   ##",
            "   ##    ###  ######   #######",
            "                              ",
            "        #######   ######      ",
            "        ##   ##  ##           ",
            "        ##   ##   ####        ",
            "        ##   ##       ##      ",
            "        #######   ######      "
        };
        
        int logo_height = 11;
        vga::Color logo_color = vga::Color(0, 255, 255); // Cyan
        
        for (int i = 0; i < logo_height; i++)
        {
            int len = stdlib::strlen(logo_lines[i]);
            for (int j = 0; j < len; j++)
            {
                if (logo_lines[i][j] != ' ')
                {
                    window_putchar(window, 
                                 start_x + j * vga::font_hdr->width, 
                                 start_y + i * vga::font_hdr->height,
                                 logo_lines[i][j], 
                                 logo_color, 
                                 vga::Color(20, 40, 80));
                }
            }
        }
    }

    void render_sysinfo_display(Window *window)
    {
        if (!window || window->type != WINDOW_TYPE_SYSINFO)
            return;

        if (!vga::font_hdr || vga::font_hdr->width == 0 || vga::font_hdr->height == 0)
            return;

        // Clear the window
        window_clear(window, vga::Color(20, 40, 80));

        int line_height = vga::font_hdr->height + 2;
        int current_y = 10;
        int left_column = 15;

        // Draw neo-OS logo at top
        draw_neo_logo(window, left_column, current_y);
        current_y += (11 * vga::font_hdr->height) + 20; // Logo height + spacing

        // Draw title
        const char* title = "System Information";
        vga::Color title_color = vga::Color(255, 255, 255);
        for (int i = 0; title[i] != '\0'; i++)
        {
            window_putchar(window, left_column + i * vga::font_hdr->width, current_y, 
                         title[i], title_color, vga::Color(20, 40, 80));
        }
        current_y += line_height + 10;

        // System information sections
        vga::Color value_color = vga::Color(255, 255, 255);
        vga::Color section_color = vga::Color(255, 255, 0);

        // Operating System section
        const char* os_section = "Operating System:";
        for (int i = 0; os_section[i] != '\0'; i++)
        {
            window_putchar(window, left_column + i * vga::font_hdr->width, current_y, 
                         os_section[i], section_color, vga::Color(20, 40, 80));
        }
        current_y += line_height;

        const char* os_name = "  Name: neo-OS v0.001A";
        for (int i = 0; os_name[i] != '\0'; i++)
        {
            window_putchar(window, left_column + i * vga::font_hdr->width, current_y, 
                         os_name[i], value_color, vga::Color(20, 40, 80));
        }
        current_y += line_height;

        const char* os_arch = "  Architecture: x86_64";
        for (int i = 0; os_arch[i] != '\0'; i++)
        {
            window_putchar(window, left_column + i * vga::font_hdr->width, current_y, 
                         os_arch[i], value_color, vga::Color(20, 40, 80));
        }
        current_y += line_height + 5;

        // Hardware section
        const char* hw_section = "Hardware:";
        for (int i = 0; hw_section[i] != '\0'; i++)
        {
            window_putchar(window, left_column + i * vga::font_hdr->width, current_y, 
                         hw_section[i], section_color, vga::Color(20, 40, 80));
        }
        current_y += line_height;

        // CPU Info
        char cpu_info[64];
        if (kernel::smp_request.response != nullptr)
        {
            // Format CPU info
            const char* cpu_base = "  CPU: x86_64 (";
            int pos = 0;
            while (cpu_base[pos] != '\0') { cpu_info[pos] = cpu_base[pos]; pos++; }
            
            // Add core count
            uint32_t cores = kernel::smp_request.response->cpu_count;
            if (cores >= 10)
            {
                cpu_info[pos++] = '0' + (cores / 10);
                cpu_info[pos++] = '0' + (cores % 10);
            }
            else
            {
                cpu_info[pos++] = '0' + cores;
            }
            
            const char* cpu_end = " cores)";
            int j = 0;
            while (cpu_end[j] != '\0') { cpu_info[pos++] = cpu_end[j++]; }
            cpu_info[pos] = '\0';
        }
        else
        {
            const char* cpu_unknown = "  CPU: Unknown";
            int i = 0;
            while (cpu_unknown[i] != '\0') { cpu_info[i] = cpu_unknown[i]; i++; }
            cpu_info[i] = '\0';WINDOW_TYPE_SYSINFO
        }

        for (int i = 0; cpu_info[i] != '\0'; i++)
        {
            window_putchar(window, left_column + i * vga::font_hdr->width, current_y, 
                         cpu_info[i], value_color, vga::Color(20, 40, 80));
        }
        current_y += line_height;

        // Memory Info
        char mem_info[64];
        uint64_t total_heap_mb = (kernel::heapBlkcount * kernel::heapBlksize) / (1024 * 1024);
        const char* mem_base = "  Memory: ";
        int pos = 0;
        while (mem_base[pos] != '\0') { mem_info[pos] = mem_base[pos]; pos++; }
        
        // Add memory amount
        if (total_heap_mb >= 1000)
        {
            mem_info[pos++] = '0' + (total_heap_mb / 1000);
            mem_info[pos++] = '0' + ((total_heap_mb % 1000) / 100);
            mem_info[pos++] = '0' + ((total_heap_mb % 100) / 10);
            mem_info[pos++] = '0' + (total_heap_mb % 10);
        }
        else if (total_heap_mb >= 100)
        {
            mem_info[pos++] = '0' + (total_heap_mb / 100);
            mem_info[pos++] = '0' + ((total_heap_mb % 100) / 10);
            mem_info[pos++] = '0' + (total_heap_mb % 10);
        }
        else if (total_heap_mb >= 10)
        {
            mem_info[pos++] = '0' + (total_heap_mb / 10);
            mem_info[pos++] = '0' + (total_heap_mb % 10);
        }
        else
        {
            mem_info[pos++] = '0' + total_heap_mb;
        }
        
        const char* mem_end = " MB heap";
        int j = 0;
        while (mem_end[j] != '\0') { mem_info[pos++] = mem_end[j++]; }
        mem_info[pos] = '\0';

        for (int i = 0; mem_info[i] != '\0'; i++)
        {
            window_putchar(window, left_column + i * vga::font_hdr->width, current_y, 
                         mem_info[i], value_color, vga::Color(20, 40, 80));
        }
        current_y += line_height + 5;

        // Display section
        const char* display_section = "Display:";
        for (int i = 0; display_section[i] != '\0'; i++)
        {
            window_putchar(window, left_column + i * vga::font_hdr->width, current_y, 
                         display_section[i], section_color, vga::Color(20, 40, 80));
        }
        current_y += line_height;

        // Resolution info
        char res_info[64];
        if (vga::fbuf_info != nullptr)
        {
            const char* res_base = "  Resolution: ";
            pos = 0;
            while (res_base[pos] != '\0') { res_info[pos] = res_base[pos]; pos++; }
            
            // Add width
            uint32_t width = vga::fbuf_info->width;
            if (width >= 1000) { res_info[pos++] = '0' + (width / 1000); width %= 1000; }
            if (width >= 100) { res_info[pos++] = '0' + (width / 100); width %= 100; }
            if (width >= 10) { res_info[pos++] = '0' + (width / 10); width %= 10; }
            res_info[pos++] = '0' + width;
            
            res_info[pos++] = 'x';
            
            // Add height
            uint32_t height = vga::fbuf_info->height;
            if (height >= 1000) { res_info[pos++] = '0' + (height / 1000); height %= 1000; }
            if (height >= 100) { res_info[pos++] = '0' + (height / 100); height %= 100; }
            if (height >= 10) { res_info[pos++] = '0' + (height / 10); height %= 10; }
            res_info[pos++] = '0' + height;
            
            res_info[pos] = '\0';
        }
        else
        {
            const char* res_unknown = "  Resolution: Unknown";
            int i = 0;
            while (res_unknown[i] != '\0') { res_info[i] = res_unknown[i]; i++; }
            res_info[i] = '\0';
        }

        for (int i = 0; res_info[i] != '\0'; i++)
        {
            window_putchar(window, left_column + i * vga::font_hdr->width, current_y, 
                         res_info[i], value_color, vga::Color(20, 40, 80));
        }
        current_y += line_height;

        // Font info
        char font_info[64];
        if (vga::font_hdr != nullptr)
        {
            const char* font_base = "  Font: PSF ";
            pos = 0;
            while (font_base[pos] != '\0') { font_info[pos] = font_base[pos]; pos++; }
            
            // Add font dimensions
            uint32_t fw = vga::font_hdr->width;
            if (fw >= 10) { font_info[pos++] = '0' + (fw / 10); fw %= 10; }
            font_info[pos++] = '0' + fw;
            font_info[pos++] = 'x';
            
            uint32_t fh = vga::font_hdr->height;
            if (fh >= 10) { font_info[pos++] = '0' + (fh / 10); fh %= 10; }
            font_info[pos++] = '0' + fh;
            
            font_info[pos] = '\0';
        }
        else
        {
            const char* font_unknown = "  Font: Unknown";
            int i = 0;
            while (font_unknown[i] != '\0') { font_info[i] = font_unknown[i]; i++; }
            font_info[i] = '\0';
        }

        for (int i = 0; font_info[i] != '\0'; i++)
        {
            window_putchar(window, left_column + i * vga::font_hdr->width, current_y, 
                         font_info[i], value_color, vga::Color(20, 40, 80));
        }
        current_y += line_height + 5;

        // System Time section
        const char* time_section = "System Time:";
        for (int i = 0; time_section[i] != '\0'; i++)
        {
            window_putchar(window, left_column + i * vga::font_hdr->width, current_y, 
                         time_section[i], section_color, vga::Color(20, 40, 80));
        }
        current_y += line_height;

        // Get current time
        rtc::DateTime dt = rtc::get_datetime();
        
        char time_info[32];
        const char* time_base = "  Current: ";
        pos = 0;
        while (time_base[pos] != '\0') { time_info[pos] = time_base[pos]; pos++; }
        
        // Add time
        time_info[pos++] = '0' + (dt.hour / 10);
        time_info[pos++] = '0' + (dt.hour % 10);
        time_info[pos++] = ':';
        time_info[pos++] = '0' + (dt.minute / 10);
        time_info[pos++] = '0' + (dt.minute % 10);
        time_info[pos++] = ' ';
        time_info[pos++] = '0' + (dt.month / 10);
        time_info[pos++] = '0' + (dt.month % 10);
        time_info[pos++] = '/';
        time_info[pos++] = '0' + (dt.day / 10);
        time_info[pos++] = '0' + (dt.day % 10);
        time_info[pos++] = '/';
        time_info[pos++] = '0' + ((dt.year / 1000) % 10);
        time_info[pos++] = '0' + ((dt.year / 100) % 10);
        time_info[pos++] = '0' + ((dt.year / 10) % 10);
        time_info[pos++] = '0' + (dt.year % 10);
        time_info[pos] = '\0';

        for (int i = 0; time_info[i] != '\0'; i++)
        {
            window_putchar(window, left_column + i * vga::font_hdr->width, current_y, 
                         time_info[i], value_color, vga::Color(20, 40, 80));
        }
        current_y += line_height;

        // Uptime
        char uptime_info[32];
        const char* uptime_base = "  Uptime: ";
        pos = 0;
        while (uptime_base[pos] != '\0') { uptime_info[pos] = uptime_base[pos]; pos++; }
        
        uint64_t uptime_seconds = millis_since_boot / 1000;
        uint64_t hours = uptime_seconds / 3600;
        uint64_t minutes = (uptime_seconds % 3600) / 60;
        uint64_t seconds = uptime_seconds % 60;
        
        // Add hours
        if (hours >= 10) { uptime_info[pos++] = '0' + (hours / 10); hours %= 10; }
        uptime_info[pos++] = '0' + hours;
        uptime_info[pos++] = 'h';
        uptime_info[pos++] = ' ';
        
        // Add minutes
        if (minutes >= 10) { uptime_info[pos++] = '0' + (minutes / 10); minutes %= 10; }
        uptime_info[pos++] = '0' + minutes;
        uptime_info[pos++] = 'm';
        uptime_info[pos++] = ' ';
        
        // Add seconds
        if (seconds >= 10) { uptime_info[pos++] = '0' + (seconds / 10); seconds %= 10; }
        uptime_info[pos++] = '0' + seconds;
        uptime_info[pos++] = 's';
        uptime_info[pos] = '\0';

        for (int i = 0; uptime_info[i] != '\0'; i++)
        {
            window_putchar(window, left_column + i * vga::font_hdr->width, current_y, 
                         uptime_info[i], value_color, vga::Color(20, 40, 80));
        }

        // Draw border
        vga::Color border_color = vga::Color(100, 150, 255);
        window_fillrect(window, 5, 5, window->width - 10, 2, border_color); // Top
        window_fillrect(window, 5, window->height - 7, window->width - 10, 2, border_color); // Bottom
        window_fillrect(window, 5, 5, 2, window->height - 10, border_color); // Left
        window_fillrect(window, window->width - 7, 5, 2, window->height - 10, border_color); // Right
    }

    // Simple random number generator for food placement
    static uint32_t snake_seed = 12345;
    uint32_t snake_rand()
    {
        snake_seed = snake_seed * 1103515245 + 12345;
        return snake_seed;
    }

    Window *create_snake_window(int x, int y, uint32_t width, uint32_t height)
    {
        log::d("WindowManager", "Creating snake game window...");
        Window *window = create_window(x, y, width, height, "Snake Game");
        if (!window)
        {
            log::e("WindowManager", "Failed to create base window for snake game");
            return nullptr;
        }

        window->type = WINDOW_TYPE_SNAKE;

        // Initialize snake game data
        window->snake_data.grid_size = 12;  // 12x12 pixel grid cells
        int grid_cols = window->width / window->snake_data.grid_size;
        int grid_rows = window->height / window->snake_data.grid_size;
        
        // Initialize snake in center
        window->snake_data.snake_length = 3;
        window->snake_data.snake_x[0] = grid_cols / 2;
        window->snake_data.snake_y[0] = grid_rows / 2;
        window->snake_data.snake_x[1] = grid_cols / 2 - 1;
        window->snake_data.snake_y[1] = grid_rows / 2;
        window->snake_data.snake_x[2] = grid_cols / 2 - 2;
        window->snake_data.snake_y[2] = grid_rows / 2;
        
        window->snake_data.direction = 1; // Start moving right
        window->snake_data.score = 0;
        window->snake_data.game_over = false;
        window->snake_data.paused = false;
        
        // Place first food
        window->snake_data.food_x = (snake_rand() % (grid_cols - 2)) + 1;
        window->snake_data.food_y = (snake_rand() % (grid_rows - 2)) + 1;

        // Clear window with dark green background
        window_clear(window, vga::Color(0, 100, 0));

        // Initial render
        render_snake_game(window);

        log::d("WindowManager", "Snake game window created successfully");
        return window;
    }

    void update_snake_game(Window *window)
    {
        if (!window || window->type != WINDOW_TYPE_SNAKE || 
            window->snake_data.game_over || window->snake_data.paused)
            return;

        int grid_cols = window->width / window->snake_data.grid_size;
        int grid_rows = window->height / window->snake_data.grid_size;

        // Calculate new head position
        int new_head_x = window->snake_data.snake_x[0];
        int new_head_y = window->snake_data.snake_y[0];

        switch (window->snake_data.direction)
        {
        case 0: new_head_y--; break; // Up
        case 1: new_head_x++; break; // Right
        case 2: new_head_y++; break; // Down
        case 3: new_head_x--; break; // Left
        }

        // Check wall collision
        if (new_head_x < 0 || new_head_x >= grid_cols || 
            new_head_y < 0 || new_head_y >= grid_rows)
        {
            window->snake_data.game_over = true;
            return;
        }

        // Check self collision
        for (int i = 0; i < window->snake_data.snake_length; i++)
        {
            if (window->snake_data.snake_x[i] == new_head_x && 
                window->snake_data.snake_y[i] == new_head_y)
            {
                window->snake_data.game_over = true;
                return;
            }
        }

        // Check food collision
        bool ate_food = false;
        if (new_head_x == window->snake_data.food_x && new_head_y == window->snake_data.food_y)
        {
            ate_food = true;
            window->snake_data.score++;
            window->snake_data.snake_length++;
            
            // Generate new food position
            do {
                window->snake_data.food_x = (snake_rand() % (grid_cols - 2)) + 1;
                window->snake_data.food_y = (snake_rand() % (grid_rows - 2)) + 1;
                
                // Make sure food doesn't spawn on snake
                bool on_snake = false;
                for (int i = 0; i < window->snake_data.snake_length; i++)
                {
                    if (window->snake_data.snake_x[i] == window->snake_data.food_x &&
                        window->snake_data.snake_y[i] == window->snake_data.food_y)
                    {
                        on_snake = true;
                        break;
                    }
                }
                if (!on_snake) break;
            } while (true);
        }

        // Move snake body
        if (!ate_food)
        {
            // Shift all segments back
            for (int i = window->snake_data.snake_length - 1; i > 0; i--)
            {
                window->snake_data.snake_x[i] = window->snake_data.snake_x[i - 1];
                window->snake_data.snake_y[i] = window->snake_data.snake_y[i - 1];
            }
        }
        else
        {
            // If ate food, shift all segments back but don't remove tail
            for (int i = window->snake_data.snake_length - 1; i > 0; i--)
            {
                window->snake_data.snake_x[i] = window->snake_data.snake_x[i - 1];
                window->snake_data.snake_y[i] = window->snake_data.snake_y[i - 1];
            }
        }

        // Update head position
        window->snake_data.snake_x[0] = new_head_x;
        window->snake_data.snake_y[0] = new_head_y;
    }

    void render_snake_game(Window *window)
    {
        if (!window || window->type != WINDOW_TYPE_SNAKE)
            return;

        // Clear background
        window_clear(window, vga::Color(0, 80, 0)); // Dark green

        int grid_size = window->snake_data.grid_size;

        // Draw snake
        vga::Color snake_color = vga::Color(0, 255, 0); // Bright green
        vga::Color head_color = vga::Color(255, 255, 0); // Yellow head
        
        for (int i = 0; i < window->snake_data.snake_length; i++)
        {
            int x = window->snake_data.snake_x[i] * grid_size;
            int y = window->snake_data.snake_y[i] * grid_size;
            
            vga::Color color = (i == 0) ? head_color : snake_color;
            window_fillrect(window, x + 1, y + 1, grid_size - 2, grid_size - 2, color);
        }

        // Draw food
        if (!window->snake_data.game_over)
        {
            int food_x = window->snake_data.food_x * grid_size;
            int food_y = window->snake_data.food_y * grid_size;
            window_fillrect(window, food_x + 2, food_y + 2, grid_size - 4, grid_size - 4, vga::Color(255, 0, 0)); // Red food
        }

        // Draw score and controls
        if (vga::font_hdr && vga::font_hdr->width > 0 && vga::font_hdr->height > 0)
        {
            char score_text[32];
            const char* score_base = "Score: ";
            int pos = 0;
            while (score_base[pos] != '\0') { score_text[pos] = score_base[pos]; pos++; }
            
            // Add score number
            int score = window->snake_data.score;
            if (score >= 100) { score_text[pos++] = '0' + (score / 100); score %= 100; }
            if (score >= 10) { score_text[pos++] = '0' + (score / 10); score %= 10; }
            score_text[pos++] = '0' + score;
            score_text[pos] = '\0';
            
            // Draw score in top-left corner
            for (int i = 0; score_text[i] != '\0'; i++)
            {
                window_putchar(window, 5 + i * vga::font_hdr->width, 5, 
                             score_text[i], vga::Color(255, 255, 255), vga::Color(0, 80, 0));
            }
            
            // Draw controls hint in bottom-right corner
            const char* controls_hint = "WASD";
            int hint_x = window->width - (stdlib::strlen(controls_hint) * vga::font_hdr->width) - 5;
            int hint_y = window->height - vga::font_hdr->height - 5;
            
            for (int i = 0; controls_hint[i] != '\0'; i++)
            {
                window_putchar(window, hint_x + i * vga::font_hdr->width, hint_y, 
                             controls_hint[i], vga::Color(150, 150, 150), vga::Color(0, 80, 0));
            }
        }

        // Draw game over message
        if (window->snake_data.game_over)
        {
            const char* game_over_text = "GAME OVER!";
            const char* restart_text = "Press R to restart";
            
            if (vga::font_hdr && vga::font_hdr->width > 0 && vga::font_hdr->height > 0)
            {
                int text_x = (window->width - stdlib::strlen(game_over_text) * vga::font_hdr->width) / 2;
                int text_y = window->height / 2 - vga::font_hdr->height;
                
                for (int i = 0; game_over_text[i] != '\0'; i++)
                {
                    window_putchar(window, text_x + i * vga::font_hdr->width, text_y, 
                                 game_over_text[i], vga::Color(255, 0, 0), vga::Color(0, 80, 0));
                }
                
                text_x = (window->width - stdlib::strlen(restart_text) * vga::font_hdr->width) / 2;
                text_y += vga::font_hdr->height + 5;
                
                for (int i = 0; restart_text[i] != '\0'; i++)
                {
                    window_putchar(window, text_x + i * vga::font_hdr->width, text_y, 
                                 restart_text[i], vga::Color(255, 255, 255), vga::Color(0, 80, 0));
                }
            }
        }

        // Draw pause message
        if (window->snake_data.paused && !window->snake_data.game_over)
        {
            const char* pause_text = "PAUSED";
            const char* continue_text = "Press SPACE to continue";
            const char* controls_text = "Use WASD to move";
            
            if (vga::font_hdr && vga::font_hdr->width > 0 && vga::font_hdr->height > 0)
            {
                int text_x = (window->width - stdlib::strlen(pause_text) * vga::font_hdr->width) / 2;
                int text_y = window->height / 2 - vga::font_hdr->height * 2;
                
                for (int i = 0; pause_text[i] != '\0'; i++)
                {
                    window_putchar(window, text_x + i * vga::font_hdr->width, text_y, 
                                 pause_text[i], vga::Color(255, 255, 0), vga::Color(0, 80, 0));
                }
                
                text_x = (window->width - stdlib::strlen(continue_text) * vga::font_hdr->width) / 2;
                text_y += vga::font_hdr->height + 5;
                
                for (int i = 0; continue_text[i] != '\0'; i++)
                {
                    window_putchar(window, text_x + i * vga::font_hdr->width, text_y, 
                                 continue_text[i], vga::Color(255, 255, 255), vga::Color(0, 80, 0));
                }
                
                text_x = (window->width - stdlib::strlen(controls_text) * vga::font_hdr->width) / 2;
                text_y += vga::font_hdr->height + 5;
                
                for (int i = 0; controls_text[i] != '\0'; i++)
                {
                    window_putchar(window, text_x + i * vga::font_hdr->width, text_y, 
                                 controls_text[i], vga::Color(200, 200, 200), vga::Color(0, 80, 0));
                }
            }
        }
    }

    Window *get_window_at_position(int x, int y)
    {
        // Check windows from top to bottom (reverse order)
        for (int i = window_count - 1; i >= 0; i--)
        {
            Window *window = windows[i];
            if (!window || !window->visible)
                continue;

            // Check if position is within window bounds (including title bar)
            if (x >= window->x - 2 && x <= window->x + (int)window->width + 2 &&
                y >= window->y - 20 && y <= window->y + (int)window->height + 2)
            {
                return window;
            }
        }
        return nullptr;
    }

    bool is_in_title_bar(Window *window, int x, int y)
    {
        if (!window)
            return false;

        return (x >= window->x - 2 && x <= window->x + (int)window->width + 2 &&
                y >= window->y - 20 && y <= window->y - 2);
    }

    bool is_in_resize_area(Window *window, int x, int y)
    {
        if (!window)
            return false;

        // Bottom-right corner resize area (10x10 pixels)
        return (x >= window->x + (int)window->width - 10 &&
                x <= window->x + (int)window->width + 2 &&
                y >= window->y + (int)window->height - 10 &&
                y <= window->y + (int)window->height + 2);
    }

    void activate_window_by_function_key(int function_key)
    {
        // F1-F11 activate windows 1-11 (F12 is reserved for closing windows)
        int window_index = function_key - 1;
        if (window_index >= 0 && window_index < window_count && function_key <= 11)
        {
            Window* target_window = windows[window_index];
            bring_window_to_top(target_window);
            set_window_focus(target_window);
        }
    }

    void handle_window_special_key(uint8_t special_key)
    {
        switch (special_key)
        {
        case 1: // Up arrow - move focused window up
            if (focused_window)
            {
                int new_y = focused_window->y - 20;
                if (new_y < 30) new_y = 30; // Keep within screen bounds
                move_window(focused_window, focused_window->x, new_y);
            }
            break;
        case 2: // Down arrow - move focused window down
            if (focused_window)
            {
                int new_y = focused_window->y + 20;
                if (vga::fbuf_info && new_y + (int)focused_window->height > (int)vga::fbuf_info->height - 20)
                    new_y = (int)vga::fbuf_info->height - (int)focused_window->height - 20;
                move_window(focused_window, focused_window->x, new_y);
            }
            break;
        case 3: // Left arrow - move focused window left
            if (focused_window)
            {
                int new_x = focused_window->x - 20;
                if (new_x < 10) new_x = 10; // Keep within screen bounds
                move_window(focused_window, new_x, focused_window->y);
            }
            break;
        case 4: // Right arrow - move focused window right
            if (focused_window)
            {
                int new_x = focused_window->x + 20;
                if (vga::fbuf_info && new_x + (int)focused_window->width > (int)vga::fbuf_info->width - 10)
                    new_x = (int)vga::fbuf_info->width - (int)focused_window->width - 10;
                move_window(focused_window, new_x, focused_window->y);
            }
            break;
        case 11: case 12: case 13: case 14: case 15: case 16: case 17: case 18: case 19: case 20: // F1-F10
        case 21: // F11
            activate_window_by_function_key(special_key - 10);
            break;
        case 22: // F12 - Close focused window
            if (focused_window)
            {
                destroy_window(focused_window);
            }
            break;
        }
        
        // Handle snake game input if focused window is a snake game
        if (focused_window && focused_window->type == WINDOW_TYPE_SNAKE)
        {
            handle_snake_input(focused_window, special_key);
        }
    }

    void handle_window_keyboard_input(char key, bool shift, bool ctrl, bool alt)
    {
        // Handle keyboard shortcuts for window management
        if (ctrl)
        {
            switch (key)
            {
            case 't': // Ctrl+T - New terminal window
            {
                int new_x = (window_count * 50) % 300 + 50;
                int new_y = (window_count * 50) % 200 + 50;
                Window *new_terminal = create_terminal_window(new_x, new_y, 600, 400);
                if (new_terminal)
                {
                    set_window_focus(new_terminal);
                    terminal_window_puts(new_terminal, "Welcome to windowed terminal!\n");
                    terminal_window_puts(new_terminal, "Type 'help' for commands\n");
                }
                break;
            }
            case 'w': // Ctrl+W - Close focused window
                if (focused_window)
                {
                    destroy_window(focused_window);
                }
                break;
            }
        }
        else if (focused_window)
        {
            // Handle snake game keyboard input
            if (focused_window->type == WINDOW_TYPE_SNAKE)
            {
                if (key == ' ') // Spacebar for pause/unpause
                {
                    focused_window->snake_data.paused = !focused_window->snake_data.paused;
                    render_snake_game(focused_window);
                }
                else if (key == 'r' || key == 'R') // R for restart
                {
                    if (focused_window->snake_data.game_over)
                    {
                        // Restart the game
                        int grid_cols = focused_window->width / focused_window->snake_data.grid_size;
                        int grid_rows = focused_window->height / focused_window->snake_data.grid_size;
                        
                        focused_window->snake_data.snake_length = 3;
                        focused_window->snake_data.snake_x[0] = grid_cols / 2;
                        focused_window->snake_data.snake_y[0] = grid_rows / 2;
                        focused_window->snake_data.snake_x[1] = grid_cols / 2 - 1;
                        focused_window->snake_data.snake_y[1] = grid_rows / 2;
                        focused_window->snake_data.snake_x[2] = grid_cols / 2 - 2;
                        focused_window->snake_data.snake_y[2] = grid_rows / 2;
                        
                        focused_window->snake_data.direction = 1;
                        focused_window->snake_data.score = 0;
                        focused_window->snake_data.game_over = false;
                        focused_window->snake_data.paused = false;
                        
                        focused_window->snake_data.food_x = (snake_rand() % (grid_cols - 2)) + 1;
                        focused_window->snake_data.food_y = (snake_rand() % (grid_rows - 2)) + 1;
                        
                        render_snake_game(focused_window);
                    }
                }
                // WASD controls for snake movement
                else if (key == 'w' || key == 'W') // Up
                {
                    if (focused_window->snake_data.direction != 2) // Don't allow reverse
                        focused_window->snake_data.direction = 0;
                }
                else if (key == 's' || key == 'S') // Down  
                {
                    if (focused_window->snake_data.direction != 0) // Don't allow reverse
                        focused_window->snake_data.direction = 2;
                }
                else if (key == 'a' || key == 'A') // Left
                {
                    if (focused_window->snake_data.direction != 1) // Don't allow reverse
                        focused_window->snake_data.direction = 3;
                }
                else if (key == 'd' || key == 'D') // Right
                {
                    if (focused_window->snake_data.direction != 3) // Don't allow reverse
                        focused_window->snake_data.direction = 1;
                }
            }
            else if (focused_window->terminal_data.input_buffer)
            {
                // Send key to focused window based on type
                if (focused_window->type == WINDOW_TYPE_LOGIN)
                {
                    login_window_handle_input(focused_window, key);
                }
                else if (focused_window->type == WINDOW_TYPE_TERMINAL)
                {
                    terminal_window_putc(focused_window, key, true);
                }
            }
        }
    }
}