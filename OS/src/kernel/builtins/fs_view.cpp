#include <kernel/mem/paging.h>
#include <kernel/shell/shell.h>
#include <drivers/vga/vga.h>
#include <drivers/vga/fonts.h>
#include <drivers/vga/vga.h>
#include <drivers/fs/fat/fat.h>

using namespace filesystem;

namespace kernel
{
    void print_file_name(fat_dir_entry *entry)
    {
        if ((entry->dir_attrib & DIRECTORY) == DIRECTORY)
        {
            printf("%p%s",
                   vga::Color({100, 200, 100}).getRGB());
        }
        else
        {
            printf("%p",
                   vga::Color({100, 100, 200}).getRGB());
        }

        bool space = false, dot = false;
        for (int i = 0; i < 11; i++)
        {
            if (entry->dir_name[i] == ' ')
            {
                space = true;
                continue;
            }

            if (space && entry->dir_name[i] != ' ' && !dot)
            {
                printf(".");
                dot = true;
            }

            printf("%c", entry->dir_name[i]);
        }

        printf("%p  ", vga::Color({255, 255, 255}).getRGB());
    }

    void list_files(const char *path)
    {
        fat_dir_entry *current_entry;
        file_handle file;
        stdlib::string file_path = path;
        int ret = kernel::open(&file, &file_path);
        if (ret == -1)
        {
            printf("Directory not found: %s\n", path);    
            return;
        }

        if (!file.is_dir)
        {
            printf("Not a directory: %s\n", path);
            return;
        }

        current_entry = (fat_dir_entry *)kernel::read(&file);
        
        void *buffer = current_entry;
        while (true)
        {
            if (current_entry->dir_name[0] == 0x00)
                break;

            if (!((uint8_t)current_entry->dir_name[0] == 0xE5 || current_entry->dir_attrib == LONG_NAME))
            {
                print_file_name(current_entry);
            }
            current_entry++;
        }
        printf("\n");

        if (!file.is_root) kernel::free_pages(buffer);
        kernel::close(&file);
    }

    void print_file_contents(const char *path)
    {
        file_handle file;
        stdlib::string file_path = path;
        int ret = kernel::open(&file, &file_path);

        if (ret == -1)
        {
            printf("Directory not found: %s\n", path);    
            return;
        }

        char* text = (char*)kernel::read(&file);

        printf("%s\n", text);

        kernel::free_pages(text);
        kernel::close(&file);
    }
}