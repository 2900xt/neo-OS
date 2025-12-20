#ifndef TAHASCRIPT_H
#define TAHASCRIPT_H
#include "stdlib/structures/pair.h"
#include <stdlib/structures/string.h>
#include <stdlib/structures/list.h>
#include <stdlib/structures/hash_map.h>
#include <kernel/vfs/file.h>

class TSRuntime
{
private:
    stdlib::list<stdlib::string> source_code;
    stdlib::hash_map<stdlib::string, void*> symbol_table;
    stdlib::list<int> stack;
    size_t next_line;
public:
    TSRuntime();
    void load_source_code(kernel::file_handle *file_input);
    void append_source_code(const stdlib::string &code_line);
    void execute_next_line();
};

#endif // TAHASCRIPT_H