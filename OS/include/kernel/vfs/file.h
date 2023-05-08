#pragma once
#include <types.h>

#define FILE_READABLE       (1 << 0)
#define FILE_WRITABLE       (1 << 1)
#define FILE_EXECUTABLE     (1 << 2)

namespace VFS
{

enum file_types : uint8_t
{
    FILE,
    DIRECTORY,
    DEVICE,
};

class file_t
{
public:
    const char* filename;
    uint8_t permissions;
    file_types file_type;


    file_t* youngest_child;
    file_t* parent;
    file_t* older_sibling;
    file_t* younger_sibling;

    void* file_data;

    file_t* get_subdir(const char* sub_filename);
    file_t* create_child(const char* new_filename, enum file_types new_file_type);

};


void vfs_init();
file_t* get_root();
void chroot(file_t* new_root);
}