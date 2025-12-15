#ifndef OS_INCLUDE_KERNEL_SHELL_SHELL_H
#define OS_INCLUDE_KERNEL_SHELL_SHELL_H

namespace kernel
{
    void login_init();
    bool login_check();
    void login_prompt(bool failed = false);
    void terminal_clear();    
    void print_prompt();
    void clear_input_buffer();
    void list_files(const char *path);
    void print_file_contents(const char *path);
    void display_fetch();
    void terminal_init();
    void terminal_putc(char src, bool is_input = false);
    void terminal_puts(const char *src);
    void printf(const char *fmt, ...);
}

#endif // OS_INCLUDE_KERNEL_SHELL_SHELL_H