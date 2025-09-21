#include <kernel/kernel.h>
#include <drivers/vga/vga.h>
#include <drivers/vga/fonts.h>
#include <drivers/fs/fat/fat.h>
#include <kernel/wm/window.h>

extern uint64_t millis_since_boot;

namespace vga
{
    extern PSF_header_t *font_hdr;
    extern limine::limine_framebuffer *fbuf_info;
    extern vga::Color fg;
}

extern volatile limine::limine_smp_request smp_request;
extern uint64_t heapBlkcount;
extern uint64_t heapBlksize;

namespace kernel
{
    extern bool logged_in;
    int terminal_cols;
    int terminal_rows;
    int terminal_x = 0, terminal_y = 0;
    stdlib::spinlock_t lock = stdlib::SPIN_UNLOCKED;
    char* input_buffer;
    int input_pos = 0;
    
    // Current working directory (using char array to avoid constructor issues)
    char current_working_directory[256] = "/";

    // Helper function to resolve relative paths to absolute paths
    stdlib::string resolve_path(const char* path_input)
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
    bool change_directory(const char* path)
    {
        // Handle special cases
        if (path != nullptr && stdlib::strcmp(path, "."))
        {
            // Current directory - do nothing
            return true;
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

        if(path[0] == '/' && path[1] == '\0')
        {
            current_working_directory[0] = '/';
            current_working_directory[1] = '\0';
            return true;
        }
        
        // Regular path - resolve and validate
        stdlib::string abs_path = resolve_path(path);
        
        // Try to open the path to verify it exists and is a directory
        File test_file;
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

    void scroll_up()
    {
        // Get framebuffer information
        uint32_t *framebuffer = vga::g_framebuffer2; // Use the working framebuffer
        int width = vga::fbuf_info->width;
        int height = vga::fbuf_info->height;
        int pitch = vga::fbuf_info->pitch / 4; // Convert bytes to 32-bit pixels
        int font_height = vga::font_hdr->height;
        
        // Copy each line up by font_height pixels
        for (int y = font_height; y < height; y++)
        {
            int source_offset = y * pitch;
            int dest_offset = (y - font_height) * pitch;
            
            for (int x = 0; x < width; x++)
            {
                framebuffer[dest_offset + x] = framebuffer[source_offset + x];
            }
        }
        
        // Clear the bottom line
        for (int y = height - font_height; y < height; y++)
        {
            int offset = y * pitch;
            for (int x = 0; x < width; x++)
            {
                framebuffer[offset + x] = vga::Color(0, 0, 0).getRGB(); // Black background
            }
        }
        
        // Mark framebuffer as dirty so it gets repainted
        vga::g_framebuffer_dirty = true;
    }

    void draw_character(int x, int y, char c, bool is_input = false)
    {
        int vga_x = x * vga::font_hdr->width;
        int vga_y = y * vga::font_hdr->height;

        vga::putchar(vga_x, vga_y, c);
    
        if (is_input)
        {
            input_buffer[input_pos++] = c;
            input_buffer[input_pos] = '\0';
        }
    }

    void draw_cursor(int x, int y)
    {
        x *= vga::font_hdr->width;
        y *= vga::font_hdr->height;

        vga::fillRect(x, y, vga::fg, vga::font_hdr->width, vga::font_hdr->height);
    }

    void print_prompt()
    {
        kernel::printf("%pROOT@NEO-OS%p:%p%s%p$%p ", 
            vga::Color(0, 255, 0).getRGB(), vga::Color(0, 0, 255).getRGB(), 
            vga::Color(150, 150, 150).getRGB(), current_working_directory,
            vga::Color(0, 0, 255).getRGB(), vga::Color(255, 255, 255).getRGB());
    }

    void clear_input_buffer()
    {
        for (int i = 0; i < input_pos; i++)
        {
            input_buffer[i] = '\0';
        }
        input_pos = 0;
    }

    void terminal_clear()
    {
        terminal_x = 0;
        terminal_y = 0;
        vga::clearScreen();
        clear_input_buffer();
    }

    void run_command(const char *command)
    {
        if (!kernel::logged_in)
        {
            kernel::login_check();
            return;
        }

        stdlib::string command_str(command);
        int count;
        stdlib::string** sp = command_str.split(' ', &count);
        //log::d(kernel_tag, "Command: %d", count);
        if (stdlib::strcmp(sp[0]->c_str(), "help"))
        {
            terminal_puts("Available commands:\n");
            terminal_puts("help - Show this help message\n");
            terminal_puts("clear - Clear the screen\n");
            terminal_puts("exit - Exit the terminal and logout\n");
            terminal_puts("fetch - Display system information\n");
            terminal_puts("ls [path] - List files in directory\n");
            terminal_puts("cat [file] - Display file contents\n");
            terminal_puts("pwd - Print current working directory\n");
            terminal_puts("cd [path] - Change working directory\n");
            terminal_puts("open - Open a new terminal window\n");
            terminal_puts("clock - Open a digital clock window\n");
            terminal_puts("sysinfo - Open system information window\n");
            terminal_puts("snake - Play the Snake game\n");
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "clear"))
        {
            terminal_clear();
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "exit"))
        {
            kernel::login_init();
            return;
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "ls"))
        {
            // Use current directory if no path specified, otherwise resolve the path
            const char* path = (count > 1) ? sp[1]->c_str() : nullptr;
            stdlib::string resolved_path = resolve_path(path);
            kernel::list_files(resolved_path.c_str());
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "cat"))
        {
            if (count < 2)
            {
                terminal_puts("cat: missing file argument\n");
            }
            else
            {
                stdlib::string resolved_path = resolve_path(sp[1]->c_str());
                kernel::print_file_contents(resolved_path.c_str());
            }
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "pwd"))
        {
            terminal_puts(current_working_directory);
            terminal_puts("\n");
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "cd"))
        {
            const char* path = (count > 1) ? sp[1]->c_str() : "/";
            if (!change_directory(path))
            {
                terminal_puts("cd: directory not found or not a directory\n");
            }
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "fetch"))
        {
            display_fetch();
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "open"))
        {
            terminal_puts("Creating new terminal window...\n");
            // Create a new terminal window
            int new_x = (wm::window_count * 30) % 400 + 50;
            int new_y = (wm::window_count * 30) % 300 + 50;
            wm::Window *new_terminal = wm::create_terminal_window(new_x, new_y, 600, 400);
            if (new_terminal)
            {
                wm::set_window_focus(new_terminal);
                terminal_puts("New terminal window opened!\n");
            }
            else
            {
                terminal_puts("Failed to create new terminal window\n");
            }
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "clock"))
        {
            terminal_puts("Creating clock window...\n");
            // Create a new clock window
            int new_x = (wm::window_count * 20) % 300 + 100;
            int new_y = (wm::window_count * 20) % 200 + 80;
            wm::Window *clock_window = wm::create_clock_window(new_x, new_y, 220, 120);
            if (clock_window)
            {
                wm::set_window_focus(clock_window);
                terminal_puts("Digital clock window opened!\n");
            }
            else
            {
                terminal_puts("Failed to create clock window\n");
            }
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "sysinfo"))
        {
            terminal_puts("Creating system info window...\n");
            // Create a new system info window
            int new_x = (wm::window_count * 25) % 250 + 50;
            int new_y = (wm::window_count * 25) % 150 + 50;
            wm::Window *sysinfo_window = wm::create_sysinfo_window(new_x, new_y, 400, 320);
            if (sysinfo_window)
            {
                wm::set_window_focus(sysinfo_window);
                terminal_puts("System information window opened!\n");
            }
            else
            {
                terminal_puts("Failed to create system info window\n");
            }
        }
        else if (stdlib::strcmp(sp[0]->c_str(), "snake"))
        {
            terminal_puts("Starting Snake game...\n");
            // Create a new snake game window
            int new_x = (wm::window_count * 30) % 200 + 50;
            int new_y = (wm::window_count * 30) % 100 + 50;
            wm::Window *snake_window = wm::create_snake_window(new_x, new_y, 300, 240);
            if (snake_window)
            {
                wm::set_window_focus(snake_window);
                terminal_puts("Snake game started! Use WASD keys to play.\n");
            }
            else
            {
                terminal_puts("Failed to create snake game window\n");
            }
        }
        else 
        {
            terminal_puts("Command not found\n");
        }

        clear_input_buffer();
        print_prompt();
    }

    void terminal_putc(char src, bool is_input)
    {
        switch (src)
        {
            case '\r':
            case '\0':
                break;
            case '\n':
                //remove cursor
                draw_character(terminal_x, terminal_y, ' ');

                // Check if we need to scroll
                if (terminal_y >= terminal_rows - 1)
                {
                    scroll_up();
                    // Stay at the bottom line
                    terminal_x = 0;
                }
                else
                {
                    terminal_y++;
                    terminal_x = 0;
                }

                //draw cursor
                draw_cursor(terminal_x, terminal_y);

                if(is_input)
                {
                    run_command(input_buffer);
                }

                break;
            case '\t':
                //remove cursor
                draw_character(terminal_x, terminal_y, ' ');

                terminal_x += 4 - (terminal_x % 4);

                //draw cursor
                draw_cursor(terminal_x, terminal_y);

                if (is_input)
                {
                    for (int i = 0; i < 4; i++)
                    {
                        input_buffer[input_pos + i] = ' ';
                    }

                    input_pos += 4 - (input_pos % 4);
                    input_buffer[input_pos] = '\0';
                }

                break;
            case '\b':
                if (is_input)
                {
                    if (input_pos == 0) break;

                    input_pos--;
                    input_buffer[input_pos] = '\0';
                }
                
                if (terminal_x > 0)
                {
                    //remove cursor
                    draw_character(terminal_x, terminal_y, ' ');

                    terminal_x--;

                    //draw cursor
                    draw_cursor(terminal_x, terminal_y);
                }
                break;
            default:
                draw_character(terminal_x, terminal_y, src, is_input);

                //draw cursor
                terminal_x++;
                draw_cursor(terminal_x, terminal_y);
                break;
        }
    }

    void terminal_puts(const char *src)
    {
        while (*src != '\0')
        {
            terminal_putc(*src++);
        }
    }

    void terminal_init()
    {
        terminal_cols = vga::fbuf_info->width / vga::font_hdr->width;
        terminal_rows = vga::fbuf_info->height / vga::font_hdr->height;
        input_buffer = (char*)kernel::kmalloc(terminal_cols * terminal_rows + 69);

        kernel::terminal_puts("Welcome to neo OS!\n");
        kernel::terminal_puts("Type 'help' for available commands\n");
        print_prompt();
    }

    void *token;
    uint64_t num;
    double dec;
    int current;
    bool argFound;

    void printf(const char *fmt, ...)
    {
        //log::d(kernel_tag, "Printing: %s", fmt);
        stdlib::acquire_spinlock(&lock);
        current = 0;
        va_list args;
        va_start(args, fmt);

        while (fmt[current] != '\0')
        {

            if (fmt[current] != '%')
            {
                terminal_putc(fmt[current++]);
                continue;
            }

            switch (fmt[current + 1])
            {
            case 'c': // Char
            case 'C':
                argFound = true;
                num = va_arg(args, int);
                terminal_putc(num);
                break;
            case 's': // String
            case 'S':
                argFound = true;
                token = va_arg(args, void *);
                terminal_puts((char *)token);
                break;
            case 'u': // Unsigned Integer
            case 'U':
                argFound = true;
                num = va_arg(args, uint64_t);
                token = stdlib::utoa(num, 10);
                terminal_puts((char *)token);
                break;
            case 'd':
            case 'D': // Signed integer
                argFound = true;
                num = va_arg(args, int64_t);
                token = stdlib::itoa(num, 10);
                terminal_puts((char *)token);
                break;
            case 'x': // Hexadecimal unsigned int
            case 'X':
                argFound = true;
                num = va_arg(args, uint64_t);
                token = stdlib::utoa(num, 16);
                terminal_puts((char *)token);
                break;
            case 'b': // Binary
            case 'B':
                argFound = true;
                num = va_arg(args, uint64_t);
                token = stdlib::utoa(num, 2);
                terminal_puts((char *)token);
                break;
            case 'f':
            case 'F':
                dec = va_arg(args, double);
                token = stdlib::dtoa(dec, 5);
                argFound = true;

                terminal_puts((char *)token);

                kernel::kfree(token);
                break;
            case 'p': // color
                num = va_arg(args, uint32_t);
                vga::set_foreground(vga::Color(num));
                argFound = true;
                break;
            case 'a': // block
                draw_cursor(terminal_x, terminal_y);
                terminal_x++;
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
        stdlib::release_spinlock(&lock);
    }

}