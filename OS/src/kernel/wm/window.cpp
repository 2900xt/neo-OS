#include <kernel/wm/window.h>
#include <kernel/kernel.h>
#include <drivers/vga/vga.h>
#include <drivers/vga/fonts.h>
#include <stdlib/stdlib.h>
#include <stdlib/string.h>
#include <drivers/fs/fat/fat.h>
#include <drivers/ps2/ps2.h>


// External variable declarations for system info
extern uint64_t millis_since_boot;

namespace vga {
extern PSF_header_t *font_hdr;
extern limine::limine_framebuffer *fbuf_info;
} // namespace vga

namespace kernel {
// External declarations for system information
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

    // Direct access to vga::font_hdr to avoid namespace issues

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
                // Shift remaining windows
                for (int j = i; j < window_count - 1; j++)
                {
                    windows[j] = windows[j + 1];
                }
                windows[--window_count] = nullptr;
                break;
            }
        }

        // Update focus if this was the focused window
        if (focused_window == window)
        {
            focused_window = (window_count > 0) ? windows[window_count - 1] : nullptr;
            if (focused_window)
                focused_window->focused = true;
        }

        // Free terminal input buffer if it exists
        if (window->terminal_data.input_buffer)
        {
            kernel::kfree(window->terminal_data.input_buffer);
        }

        // Free framebuffer and window
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

        // Allocate new framebuffer
        uint32_t new_buffer_size = new_width * new_height * sizeof(uint32_t);
        uint32_t *new_framebuffer = (uint32_t *)kernel::kmalloc(new_buffer_size);
        if (!new_framebuffer)
            return;

        // Clear new framebuffer
        kernel::memset_8(new_framebuffer, new_buffer_size, 0);

        // Copy old content (as much as fits)
        uint32_t copy_width = (new_width < window->width) ? new_width : window->width;
        uint32_t copy_height = (new_height < window->height) ? new_height : window->height;

        for (uint32_t y = 0; y < copy_height; y++)
        {
            for (uint32_t x = 0; x < copy_width; x++)
            {
                new_framebuffer[y * new_width + x] = window->framebuffer[y * window->width + x];
            }
        }

        // Replace old framebuffer
        kernel::kfree(window->framebuffer);
        window->framebuffer = new_framebuffer;
        window->width = new_width;
        window->height = new_height;

        // Update terminal data if this is a terminal window
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

        // Remove focus from current window
        if (focused_window)
        {
            focused_window->focused = false;
        }

        // Set new focus
        focused_window = window;
        window->focused = true;
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

        // Get bitmap data pointer similar to fonts.cpp
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

        // Window title
        if (vga::font_hdr && vga::font_hdr->width > 0)
        {
            int title_x = window->x;
            int title_y = window->y - 20;
            for (int i = 0; window->title[i] != '\0' && i < 20; i++)
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
        if (render_count < 5) // Only log first few renders to avoid spam
        {
            log::d("WindowManager", "Rendering all windows (count=%d)", window_count);
            render_count++;
        }

        // Clear screen with cyan blue backdrop
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

        // Mark screen as dirty for repaint
        vga::g_framebuffer_dirty = true;
    }

    void draw_mouse_cursor()
    {
        int x = ps2::mouse_state.x;
        int y = ps2::mouse_state.y;

        // Draw a simple arrow cursor
        vga::Color cursor_color = vga::Color(255, 255, 255);
        vga::Color outline_color = vga::Color(0, 0, 0);

        // Simple arrow cursor pattern
        for (int dy = 0; dy < 16; dy++)
        {
            for (int dx = 0; dx < 10; dx++)
            {
                bool draw_pixel = false;
                bool draw_outline = false;

                // Simple arrow shape
                if (dy < 10 && dx <= dy)
                {
                    draw_pixel = true;
                }
                else if (dy >= 10 && dy < 14 && dx >= 4 && dx <= 6)
                {
                    draw_pixel = true;
                }

                // Outline
                if (draw_pixel)
                {
                    if (dx == 0 || dy == 0 || dx == dy || (dy >= 10 && dy < 14 && (dx == 4 || dx == 6)))
                    {
                        draw_outline = true;
                    }
                }

                if (x + dx < (int)vga::fbuf_info->width && y + dy < (int)vga::fbuf_info->height)
                {
                    if (draw_outline)
                    {
                        vga::putpixel(x + dx, y + dy, outline_color);
                    }
                    else if (draw_pixel)
                    {
                        vga::putpixel(x + dx, y + dy, cursor_color);
                    }
                }
            }
        }
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
        case '\0':
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

    void handle_window_mouse_input(int mouse_x, int mouse_y, bool left_click, bool right_click)
    {
        static bool last_left_click = false;

        // Handle drag/resize release
        if (!left_click && last_left_click)
        {
            interaction_state.dragging = false;
            interaction_state.resizing = false;
            interaction_state.active_window = nullptr;
        }

        // Handle new clicks
        if (left_click && !last_left_click)
        {
            Window *clicked_window = get_window_at_position(mouse_x, mouse_y);

            if (clicked_window)
            {
                // Set focus to clicked window
                set_window_focus(clicked_window);

                // Check if clicking in resize area
                if (is_in_resize_area(clicked_window, mouse_x, mouse_y))
                {
                    interaction_state.resizing = true;
                    interaction_state.active_window = clicked_window;
                    interaction_state.resize_start_width = clicked_window->width;
                    interaction_state.resize_start_height = clicked_window->height;
                }
                // Check if clicking in title bar for dragging
                else if (is_in_title_bar(clicked_window, mouse_x, mouse_y))
                {
                    interaction_state.dragging = true;
                    interaction_state.active_window = clicked_window;
                    interaction_state.drag_offset_x = mouse_x - clicked_window->x;
                    interaction_state.drag_offset_y = mouse_y - clicked_window->y;
                }
            }
        }

        // Handle dragging
        if (interaction_state.dragging && interaction_state.active_window)
        {
            int new_x = mouse_x - interaction_state.drag_offset_x;
            int new_y = mouse_y - interaction_state.drag_offset_y;
            move_window(interaction_state.active_window, new_x, new_y);
        }

        // Handle resizing
        if (interaction_state.resizing && interaction_state.active_window)
        {
            Window *window = interaction_state.active_window;
            int new_width = mouse_x - window->x;
            int new_height = mouse_y - window->y;

            // Minimum window size
            if (new_width < 100)
                new_width = 100;
            if (new_height < 50)
                new_height = 50;

            resize_window(window, new_width, new_height);
        }

        last_left_click = left_click;
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
        else if (focused_window && focused_window->terminal_data.input_buffer)
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