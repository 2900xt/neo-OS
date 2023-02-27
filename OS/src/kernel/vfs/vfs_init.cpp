#include "stdlib/assert.h"
#include <kernel/vfs/file.h>
#include <stdlib/string.h>
#include <stdlib/stdio.h>

namespace VFS
{

static file_t root;

file_t* dev;
file_t* run;
file_t* proc;
file_t* sys;
file_t* etc;
file_t* usr;
file_t* var;
file_t* opt;
file_t* mnt;

file_t* get_root()
{
    return &root;
}


void chroot(file_t* new_root)
{
    if(new_root->file_type != VFS::DIRECTORY) return;

    root = *new_root;
    root.older_sibling = NULL;
    root.younger_sibling = NULL;
    root.parent = NULL;
    root.filename = "!#";
}

void root_init()
{
    root.file_type = VFS::DIRECTORY;
    root.filename = "!#";
    root.older_sibling = NULL;
    root.younger_sibling = NULL;
    root.parent = NULL;
    root.youngest_child = NULL;
    root.permissions = 255;
}

void list_files(file_t* directory)
{
    assert(directory->file_type == DIRECTORY);
    file_t* current_file = directory->youngest_child;
    while(true)
    {
        if(current_file == NULL) break;
        std::klogf("%s\\\t", current_file->filename);
        current_file = current_file->older_sibling;
    }
    std::klogf("\n");
}

void vfs_init()
{

    root_init();
    std::klogf("Initializing VFS...\n");

    dev = get_root()->create_child("dev", VFS::DIRECTORY);
    run = get_root()->create_child("run", VFS::DIRECTORY);
    sys = get_root()->create_child("sys", VFS::DIRECTORY);
    etc = get_root()->create_child("etc", VFS::DIRECTORY);
    usr = get_root()->create_child("usr", VFS::DIRECTORY);
    var = get_root()->create_child("var", VFS::DIRECTORY);
    opt = get_root()->create_child("opt", VFS::DIRECTORY);
    mnt = get_root()->create_child("mnt", VFS::DIRECTORY);
    proc = get_root()->create_child("proc", VFS::DIRECTORY);

    list_files(get_root());

}
}