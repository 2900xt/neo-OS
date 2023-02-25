#include <types.h>

#define RAX     registers[0]    //Offset - 0        
#define RBX     registers[1]    //Offset - 64
#define RCX     registers[2]    //Offset - 128
#define RDX     registers[3]    //Offset - 192
#define R8      registers[4]    //Offset - 256
#define R9      registers[5]    //Offset - 320
#define R10     registers[6]    //Offset - 384
#define R11     registers[7]    //Offset - 448
#define R12     registers[8]    //Offset - 512
#define R13     registers[9]    //Offset - 576
#define R14     registers[10]   //Offset - 640
#define R15     registers[11]   //Offset - 704
#define RSI     registers[12]   //Offset - 768
#define RDI     registers[13]   //Offset - 832
#define RFLAGS  registers[14]   //Offset - 896
#define RIP     registers[15]   //Offset - 960
#define RSP     registers[16]   //Offset - 1024

typedef uint64_t register_t;

typedef uint64_t pid_t;

enum process_state_t
{
    READY,
    RUNNING,
    WAITING,
    ZOMBIE,
    SLEEPING
};

struct process_t
{
    //Process context (used to suspend)

    register_t registers[17];

    //Unimplemented (I/O streams)

    void*       stdout;
    void*       stdin;
    void*       stderr;

    void**      files;

    //Relative processes

    process_t* parent;
    process_t* older_sibling;
    process_t* younger_sibling;
    process_t* oldest_child;

    //Process state and id

    process_state_t state;
    bool            isKilled;
    pid_t           pid;

    //Memory allocated to running process

    void*       mem;
    uint64_t    mem_sz;
    uint64_t*   stack;              //Stack Frame
    uint64_t    stack_sz;
};

process_t* mk_proc(process_t* parent);      //Makes a process in the 'READY' state
void       run_proc(process_t* proc);       //Schedules and runs a process. Process must be in the 'READY' state
