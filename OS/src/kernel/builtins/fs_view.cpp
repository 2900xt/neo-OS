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
        printf("Directory not found: %s\n", path);    
    }

    void print_file_contents(const char *path)
    {
        printf("Directory not found: %s\n", path);    
    }
}