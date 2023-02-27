#include "stdlib/assert.h"
#include <types.h>
#include <kernel/vfs/file.h>
#include <kernel/mem.h>
#include <kernel/x64/paging.h>
#include <stdlib/string.h>
#include <stdlib/stdio.h>

namespace VFS {

file_t* create_file(file_t* parent_dir, const char* filename, enum file_types file_type)
{

    if(parent_dir->file_type != DIRECTORY)
    {
        return NULL;
    }

    file_t* new_file = (file_t*)kcalloc(1, sizeof(file_t));
    new_file->filename = filename;
    new_file->permissions = 0;
    new_file->parent = parent_dir;
    new_file->youngest_child = NULL;

    if(parent_dir->youngest_child == NULL)
    {
        parent_dir->youngest_child = new_file;
        return new_file;
    }

    new_file->older_sibling = parent_dir->youngest_child;
    new_file->older_sibling->younger_sibling = new_file;
    parent_dir->youngest_child = new_file;
    
    return new_file;
}


file_t* file_t::get_subdir(const char* sub_filename)
{
    file_t* current_file = this->youngest_child;
    while(true)
    {
        if(std::strcmp(sub_filename, current_file->filename)) break;
        if(current_file->older_sibling == NULL) return NULL;
        current_file = current_file->older_sibling;
    }
    return current_file;
}

file_t* file_t::create_child(const char *new_filename, enum file_types new_file_type)
{
    assert(this->file_type == DIRECTORY);

    file_t* child = (file_t*)kcalloc(1, sizeof(file_t));
    child->file_data = NULL;
    child->filename = new_filename;
    child->permissions = this->permissions;
    child->parent = this;
    child->file_type = new_file_type;
    child->youngest_child = NULL;
    child->younger_sibling = NULL;

    if(this->youngest_child == NULL)
    {
        this->youngest_child = child;
        return child;
    }

    child->older_sibling = this->youngest_child;
    child->older_sibling->younger_sibling = child;
    this->youngest_child = child;
    return child;
}

}
