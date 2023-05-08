#include <types.h>
#include <kernel/vfs/file.h>

typedef uint64_t pid_t;

enum process_state_t
{
    EMBRYO,
    READY,
    RUNNING,
    WAITING,
    SLEEPING
};

class Process
{
public:

    //Executable Code pointer

    void (*code_ptr)(pid_t, int, char**);

    //Process context (used to suspend)

    uint64_t    stack_ptr;
    uint64_t*   stack_base;

    //IO streams

    VFS::file_t*       stdout;
    VFS::file_t*       stdin;
    VFS::file_t*       stderr;

    //Relative processes

    Process* parent;
    Process* older_sibling;
    Process* younger_sibling;
    Process* youngest_child;

    //Process state and id

    process_state_t state;
    const char*     pname;
    pid_t           pid;
    int             exit_code;

    Process*  create_child(const char* child_pname, void (*code)(pid_t ,int, char**));
    void      schedule(void);
};