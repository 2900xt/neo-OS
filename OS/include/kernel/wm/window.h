#ifndef WINDOW_H
#define WINDOW_H

#include <types.h>
#include <drivers/vga/vga.h>

namespace wm
{
    enum WindowType {
        WINDOW_TYPE_TERMINAL,
        WINDOW_TYPE_LOGIN,
        WINDOW_TYPE_CLOCK,
        WINDOW_TYPE_SYSINFO,
        WINDOW_TYPE_SNAKE
    };

    struct Window
    {
        uint32_t id;
        int x, y;                    // Position on screen
        uint32_t width, height;      // Dimensions
        uint32_t *framebuffer;       // Window's own framebuffer
        bool visible;                // Whether window is visible
        bool focused;                // Whether window has focus
        char title[64];              // Window title
        WindowType type;             // Type of window
        
        // Terminal-specific data
        struct {
            int cursor_x, cursor_y;
            int cols, rows;
            char* input_buffer;
            int input_pos;
            vga::Color fg_color;
            vga::Color bg_color;
        } terminal_data;
        
        // Snake game data
        struct {
            int snake_x[100], snake_y[100];  // Snake body positions
            int snake_length;
            int direction; // 0=up, 1=right, 2=down, 3=left
            int food_x, food_y;
            int score;
            bool game_over;
            bool paused;
            int grid_size;
        } snake_data;
    };

    // Window management functions
    void window_manager_init();
    Window* create_window(int x, int y, uint32_t width, uint32_t height, const char* title);
    void destroy_window(Window* window);
    void move_window(Window* window, int new_x, int new_y);
    void resize_window(Window* window, uint32_t new_width, uint32_t new_height);
    void set_window_focus(Window* window);
    void bring_window_to_top(Window* window);
    void render_window(Window* window);
    void render_all_windows();
    void draw_mouse_cursor();
    
    // Window framebuffer operations
    void window_putpixel(Window* window, int x, int y, vga::Color color);
    void window_fillrect(Window* window, int x, int y, uint32_t w, uint32_t h, vga::Color color);
    void window_putchar(Window* window, int x, int y, char c, vga::Color fg, vga::Color bg);
    void window_clear(Window* window, vga::Color bg_color);
    
    // Terminal window functions
    Window* create_terminal_window(int x, int y, uint32_t width, uint32_t height);
    
    // Login window functions
    Window* create_login_window(int x, int y, uint32_t width, uint32_t height);
    
    // Clock window functions
    Window* create_clock_window(int x, int y, uint32_t width, uint32_t height);
    void update_clock_display(Window* window);
    
    // System info window functions
    Window* create_sysinfo_window(int x, int y, uint32_t width, uint32_t height);
    void render_sysinfo_display(Window* window);
    
    // Snake game window functions
    Window* create_snake_window(int x, int y, uint32_t width, uint32_t height);
    void update_snake_game(Window* window);
    void render_snake_game(Window* window);
    void handle_snake_input(Window* window, uint8_t special_key);
    void login_window_handle_input(Window* window, char key);
    void terminal_window_putc(Window* window, char c, bool is_input = false);
    void terminal_window_puts(Window* window, const char* str);
    void terminal_window_scroll_up(Window* window);
    void terminal_window_printf(Window* window, const char* fmt, ...);
    void terminal_window_print_prompt(Window* window);
    void terminal_window_run_command(Window* window, const char* command);
    void terminal_window_display_fetch(Window* window);
    void terminal_window_print_file_contents(Window* window, const char* path);
    
    // Input handling for windows
    void handle_window_mouse_input(int mouse_x, int mouse_y, bool left_click, bool right_click);
    void handle_window_keyboard_input(char key, bool shift, bool ctrl, bool alt);
    void handle_window_special_key(uint8_t special_key);
    void cycle_windows(bool forward);
    void activate_window_by_function_key(int function_key);
    Window* get_window_at_position(int x, int y);
    bool is_in_title_bar(Window* window, int x, int y);
    bool is_in_resize_area(Window* window, int x, int y);
    
    // Window interaction state
    struct WindowInteractionState
    {
        bool dragging;
        bool resizing;
        Window* active_window;
        int drag_offset_x, drag_offset_y;
        int resize_start_width, resize_start_height;
    };
    
    // Global window manager state
    extern Window* windows[16];  // Max 16 windows
    extern int window_count;
    extern Window* focused_window;
    extern uint32_t next_window_id;
    extern WindowInteractionState interaction_state;
}

#endif