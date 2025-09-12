#include <kernel/kernel.h>
#include <drivers/vga/vga.h>
#include <drivers/vga/fonts.h>

namespace kernel
{
    bool logged_in = false;
    void login_prompt(bool failed)
    {
        if (failed)
            printf("%pPassword incorrect :(%p\n", vga::Color(150, 0, 0).getRGB(), vga::Color(255, 255, 255).getRGB());
        else 
            terminal_clear();

        terminal_puts("Password: ");
    }

    char *password;
    void login_init()
    {
        logged_in = false;
        File login_file;
        stdlib::string login_file_path = "/etc/login";
        kernel::open(&login_file, &login_file_path);

        password = (char*)kernel::read(&login_file);
        kernel::close(&login_file);

        login_prompt(false);
    }

    extern char *input_buffer;
    bool login_check()
    {
        //log::d(kernel_tag, "%s %s", password, kernel::input_buffer);
        if (stdlib::strcmp(password, kernel::input_buffer))
        {
            printf("%pLogin successful%p\n", vga::Color(0, 150, 0).getRGB(), vga::Color(255, 255, 255).getRGB());
            clear_input_buffer();
            print_prompt();
            logged_in = true;
            return true;
        }
        else
        {
            clear_input_buffer();
            login_prompt(true);
            logged_in = false;
            return false;
        }
    }
}